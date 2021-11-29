#include "WebClient_P.h"
#include "WebClient.h"

#include "QtCommon.h"
#include <QEventLoop>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>

#include "OsmAndCore_private.h"
#include "QNetworkWaitable.h"

OsmAnd::WebClient_P::WebClient_P(
    WebClient* const owner_,
    const QString& userAgent_,
    const unsigned int concurrentRequestsLimit_,
    const unsigned int retriesLimit_,
    const bool followRedirects_)
    : owner(owner_)
    , _userAgent(userAgent_)
    , _retriesLimit(retriesLimit_)
    , _followRedirects(followRedirects_ ? 1 : 0)
{
    _threadPool.setMaxThreadCount(concurrentRequestsLimit_);
}

OsmAnd::WebClient_P::~WebClient_P()
{
    _threadPool.clear();
    REPEAT_UNTIL(_threadPool.waitForDone());
}

QString OsmAnd::WebClient_P::getUserAgent() const
{
    QReadLocker scopedLocker(&_userAgentLock);

    return detachedOf(_userAgent);
}

void OsmAnd::WebClient_P::setUserAgent(const QString& userAgent)
{
    QWriteLocker scopedLocker(&_userAgentLock);

    _userAgent = detachedOf(userAgent);
}

unsigned int OsmAnd::WebClient_P::getConcurrentRequestsLimit() const
{
    return _threadPool.maxThreadCount();
}

void OsmAnd::WebClient_P::setConcurrentRequestsLimit(const unsigned int newLimit)
{
    _threadPool.setMaxThreadCount(newLimit);
}

unsigned int OsmAnd::WebClient_P::getRetriesLimit() const
{
    return _retriesLimit.loadAcquire();
}

void OsmAnd::WebClient_P::setRetriesLimit(const unsigned int newLimit)
{
    _retriesLimit.fetchAndStoreOrdered(newLimit);
}

bool OsmAnd::WebClient_P::getFollowRedirects() const
{
    return (_followRedirects.loadAcquire() == 1);
}

void OsmAnd::WebClient_P::setFollowRedirects(const bool followRedirects)
{
    _followRedirects.fetchAndStoreOrdered(followRedirects ? 1 : 0);
}

QByteArray OsmAnd::WebClient_P::downloadData(
    const QNetworkRequest& networkRequest,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult,
    const IWebClient::RequestProgressCallbackSignature progressCallback) const
{
    QByteArray data;

    const auto dataConsumer = [&data](QNetworkReply& networkReply) -> uint64_t
    {
        const auto chunk = networkReply.readAll();
        data.append(chunk);

        return chunk.size();
    };
    const Request::TransferProgressCallback downloadProgressCallback =
        [progressCallback]
        (qint64 transferredBytes, qint64 totalBytes) -> void
        {
            progressCallback(transferredBytes, totalBytes);
        };

    Request request(
        networkRequest,
        getUserAgent(),
        getRetriesLimit(),
        getFollowRedirects(),
        dataConsumer,
        progressCallback != nullptr ? downloadProgressCallback : nullptr,
        nullptr);
    _threadPool.start(&request);
    request.waitUntilFinished();

    if (request._lastNetworkReply == nullptr || request._lastNetworkReply->error() != QNetworkReply::NoError)
    {
        if (requestResult != nullptr)
            requestResult->reset(request._lastNetworkReply != nullptr ? new HttpRequestResult(request._lastNetworkReply) : nullptr);
        return QByteArray();
    }

    if (requestResult != nullptr)
        requestResult->reset(request._lastNetworkReply != nullptr ? new HttpRequestResult(request._lastNetworkReply) : nullptr);
    return data;
}

QString OsmAnd::WebClient_P::downloadString(
    const QNetworkRequest& networkRequest,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult,
    const IWebClient::RequestProgressCallbackSignature progressCallback) const
{
    QByteArray data;

    const auto dataConsumer = [&data](QNetworkReply& networkReply) -> uint64_t
    {
        const auto chunk = networkReply.readAll();
        data.append(chunk);

        return chunk.size();
    };
    const Request::TransferProgressCallback downloadProgressCallback =
        [progressCallback]
    (qint64 transferredBytes, qint64 totalBytes) -> void
    {
        progressCallback(transferredBytes, totalBytes);
    };

    Request request(
        networkRequest,
        getUserAgent(),
        getRetriesLimit(),
        getFollowRedirects(),
        dataConsumer,
        progressCallback != nullptr ? downloadProgressCallback : nullptr,
        nullptr);
    _threadPool.start(&request);
    request.waitUntilFinished();

    QString charset = {};
    if (request._lastNetworkReply->hasRawHeader("Content-Type"))
    {
        const auto contentTypeParameters = QString(request._lastNetworkReply->rawHeader("Content-Type")).split(';', Qt::SkipEmptyParts);
        for(const auto& contentTypeParam : constOf(contentTypeParameters))
        {
            const auto trimmed = contentTypeParam.trimmed();
            if (trimmed.startsWith(QLatin1String("charset=")))
            {
                charset = trimmed.mid(8).toLower();
                break;
            }
        }
    }

    if (request._lastNetworkReply == nullptr || request._lastNetworkReply->error() != QNetworkReply::NoError)
    {
        if (requestResult != nullptr)
            requestResult->reset(request._lastNetworkReply != nullptr ? new HttpRequestResult(request._lastNetworkReply) : nullptr);
        return {};
    }

    if (requestResult != nullptr)
        requestResult->reset(request._lastNetworkReply != nullptr ? new HttpRequestResult(request._lastNetworkReply) : nullptr);
    if (!charset.isNull() && charset.contains(QLatin1String("utf-8")))
        return QString::fromUtf8(data);
    return QString::fromLocal8Bit(data);
}

bool OsmAnd::WebClient_P::downloadFile(
    const QNetworkRequest& networkRequest,
    const QString& fileName,
    std::shared_ptr<const IWebClient::IRequestResult>* const requestResult,
    const IWebClient::RequestProgressCallbackSignature progressCallback) const
{
    QFile file(fileName);

    // Create entire directory structure
    QFileInfo(file).dir().mkpath(QLatin1String("."));

    // Open file for writing, replacing
    bool ok = file.open(QIODevice::WriteOnly | QIODevice::Truncate);
    if (!ok)
        return false;

    enum {
        IntermediateBufferSize = 16 * 1024
    };
    uint8_t* const intermediateBuffer = new uint8_t[IntermediateBufferSize];
    const auto dataConsumer = [&file, intermediateBuffer](QNetworkReply& networkReply) -> uint64_t
    {
        uint64_t totalBytesConsumed = 0;

        while(networkReply.bytesAvailable() > 0)
        {
            // Read some portion of data from network
            const auto bytesRead = networkReply.read(
                reinterpret_cast<char*>(intermediateBuffer),
                qMin(static_cast<qint64>(IntermediateBufferSize), networkReply.bytesAvailable()));
            if (bytesRead <= 0)
                break;

            // Write that chunk into destination completely
            uint64_t bytesWritten = 0;
            do
            {
                const auto writtenChunkSize = file.write(
                    reinterpret_cast<char*>(intermediateBuffer + bytesWritten),
                    bytesRead - bytesWritten);
                if (writtenChunkSize <= 0)
                    break;
                bytesWritten += writtenChunkSize;
            } while(bytesWritten != bytesRead);

            totalBytesConsumed += bytesWritten;
        }

        return totalBytesConsumed;
    };

    const Request::TransferProgressCallback downloadProgressCallback =
        [progressCallback]
        (qint64 transferredBytes, qint64 totalBytes) -> void
        {
            progressCallback(transferredBytes, totalBytes);
        };

    Request request(
        networkRequest,
        getUserAgent(),
        getRetriesLimit(),
        getFollowRedirects(),
        dataConsumer,
        progressCallback != nullptr ? downloadProgressCallback : nullptr,
        nullptr);
    _threadPool.start(&request);
    request.waitUntilFinished();

    delete[] intermediateBuffer;

    if (request._lastNetworkReply == nullptr || request._lastNetworkReply->error() != QNetworkReply::NoError)
    {
        file.close();
        file.remove();

        if (requestResult != nullptr)
            requestResult->reset(request._lastNetworkReply != nullptr ? new HttpRequestResult(request._lastNetworkReply) : nullptr);
        return false;
    }

    file.flush();
    file.close();

    if (requestResult != nullptr)
        requestResult->reset(request._lastNetworkReply != nullptr ? new HttpRequestResult(request._lastNetworkReply) : nullptr);
    return true;
}

OsmAnd::WebClient_P::Request::Request(
    const QNetworkRequest& networkRequest_,
    const QString& userAgent_,
    const unsigned int retriesLimit_,
    const bool followRedirects_,
    const DataConsumer dataConsumer_,
    const TransferProgressCallback downloadProgressCallback_,
    const TransferProgressCallback uploadProgressCallback_)
    : _lastNetworkReply(nullptr)
    , _totalBytesConsumed(0)
    , originalNetworkRequest(networkRequest_)
    , userAgent(userAgent_)
    , retriesLimit(retriesLimit_)
    , followRedirects(followRedirects_)
    , dataConsumer(dataConsumer_)
    , downloadProgressCallback(downloadProgressCallback_)
    , uploadProgressCallback(uploadProgressCallback_)
{
    setAutoDelete(false);
}

OsmAnd::WebClient_P::Request::~Request()
{
    if (_lastNetworkReply != nullptr)
        _lastNetworkReply->deleteLater();
}

void OsmAnd::WebClient_P::Request::run()
{
    QEventLoop eventLoop;
    QNetworkAccessManager networkAccessManager;
    QNetworkWaitable waitable(&networkAccessManager, eventLoop);
    
    // Configure network request
#if defined(QT_NO_COMPRESS)
#   error QT_NO_COMPRESS is defined, but expected to have compression support
#endif // defined(QT_NO_COMPRESS)
/*
#if defined(QT_NO_SSL)
#   error QT_NO_SSL is defined, but expected to have HTTPS support
#endif // defined(QT_NO_SSL)
*/
    auto networkRequest = originalNetworkRequest;
    networkRequest.setHeader(QNetworkRequest::UserAgentHeader, userAgent.toLatin1());

    // Obtain network reply
    auto networkReply = networkAccessManager.get(networkRequest);
    if (followRedirects)
    {
        for(;;)
        {
            // Wait for reply headers to be ready. This will happen or when finished() signal is emmited,
            // or when first byte is ready to be read.
            waitable.waitForMetaDataOf(networkReply, false);

            // If any error have occurred, do nothing
            if (networkReply->error() != QNetworkReply::NoError)
                break;

            const auto redirectionTarget = networkReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (!redirectionTarget.isEmpty())
            {
                // Delete current reply
                waitable.reset(networkReply);
                networkReply->deleteLater();

                // Create new network request, and process it
                networkRequest.setUrl(redirectionTarget);
                networkReply = networkAccessManager.get(networkRequest);
                continue;
            }

            break;
        }
    }

    // Check if "Range" header is supported for requesting partial content
    bool rangeHeaderSupported = false;
    uint64_t contentSize = 0;
    if (networkReply->hasRawHeader("Accept-Ranges") &&
        QString(networkReply->rawHeader("Accept-Ranges")).contains(QLatin1String("bytes")) &&
        networkReply->hasRawHeader("Content-Length"))
    {
        contentSize = QString(networkReply->rawHeader("Content-Length")).toULongLong(&rangeHeaderSupported);
    }
    if (!rangeHeaderSupported)
    {
        auto rangeNetworkRequest = networkRequest;
        rangeNetworkRequest.setRawHeader("Range", "bytes=0-");

        auto rangeNetworkReply = networkAccessManager.head(rangeNetworkRequest);

        // Wait for reply headers to be ready. This will happen or when finished() signal is emmited,
        // or when first byte is ready to be read.
        waitable.waitForMetaDataOf(rangeNetworkReply, true);

        // Check response
        if (rangeNetworkReply->error() == QNetworkReply::NoError)
        {
            // Try to find "Content-Range" header
            if (networkReply->hasRawHeader("Content-Range"))
            {
                const QString contentRangeHeader(networkReply->rawHeader("Content-Range"));
                QRegExp contentRangeHeaderParser(QLatin1String("bytes (\\d+)-(\\d+)\\/.*"));
                if (contentRangeHeaderParser.exactMatch(contentRangeHeader))
                    contentSize = contentRangeHeaderParser.cap(2).toULongLong(&rangeHeaderSupported);
            }
        }

        rangeNetworkReply->deleteLater();
    }

    // Prepare for processing and launch it
    unsigned int retriesCount = 0;
    _lastNetworkReply = networkReply;
    do 
    {
        const auto downloadProgressConnection = QObject::connect(
            networkReply, &QNetworkReply::downloadProgress,
            (std::function<void(qint64, qint64)>)std::bind(&Request::onDownloadProgress, this, std::placeholders::_1, std::placeholders::_2));
        const auto uploadProgressConnection = QObject::connect(
            networkReply, &QNetworkReply::uploadProgress,
            (std::function<void(qint64, qint64)>)std::bind(&Request::onUploadProgress, this, std::placeholders::_1, std::placeholders::_2));
        waitable.waitForFinishOf(_lastNetworkReply, true);
        QObject::disconnect(downloadProgressConnection);
        QObject::disconnect(uploadProgressConnection);

        // If there was no error, just quit retry loop
        if (networkReply->error() == QNetworkReply::NoError)
            break;

        // If "Range" header is not supported, it's impossible to resume download
        if (_totalBytesConsumed > 0 && !rangeHeaderSupported)
            break;

        // Otherwise, retry download
        _lastNetworkReply = nullptr;
        networkReply->deleteLater();
        if (_totalBytesConsumed > 0)
        {
            assert(_totalBytesConsumed < contentSize);
            const auto rangeHeaderValue = QString(QLatin1String("bytes=%1-%2")).arg(_totalBytesConsumed).arg(contentSize - 1);
            networkRequest.setRawHeader("Range", rangeHeaderValue.toLocal8Bit());
        }
        _lastNetworkReply = networkReply = networkAccessManager.get(networkRequest);

        retriesCount++;
    } while(retriesCount < retriesLimit);

    // Process remaining data if such exists
    _totalBytesConsumed += dataConsumer(*_lastNetworkReply);

    // Detach network reply from it's parent
    _lastNetworkReply->setParent(nullptr);

    // Done
    {
        QMutexLocker scopedLocker(&_finishedConditionMutex);
        _finishedCondition.wakeOne();
    }
}

void OsmAnd::WebClient_P::Request::onReadyRead()
{
    _totalBytesConsumed += dataConsumer(*_lastNetworkReply);
}

void OsmAnd::WebClient_P::Request::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    if (downloadProgressCallback)
        downloadProgressCallback(bytesReceived, bytesTotal);
}

void OsmAnd::WebClient_P::Request::onUploadProgress(qint64 bytesSent, qint64 bytesTotal)
{
    if (uploadProgressCallback)
        uploadProgressCallback(bytesSent, bytesTotal);
}

void OsmAnd::WebClient_P::Request::waitUntilFinished() const
{
    QMutexLocker scopedLocker(&_finishedConditionMutex);
    REPEAT_UNTIL(_finishedCondition.wait(&_finishedConditionMutex));
}
