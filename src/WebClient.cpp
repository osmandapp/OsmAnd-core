#include "WebClient.h"
#include "WebClient_P.h"

OsmAnd::WebClient::WebClient(
    const QString& userAgent /*= QString::null*/,
    const unsigned int concurrentRequestsLimit /*= 1*/,
    const unsigned int retriesLimit /*= 1*/,
    const bool followRedirects /*= true*/)
    : _p(new WebClient_P(this, userAgent, concurrentRequestsLimit, retriesLimit, followRedirects))
{
}

OsmAnd::WebClient::~WebClient()
{
}

const QString OsmAnd::WebClient::BuiltinUserAgent(QLatin1String("OsmAnd Core"));

QString OsmAnd::WebClient::getDefaultUserAgent()
{
    return WebClient_P::getDefaultUserAgent();
}

void OsmAnd::WebClient::setDefaultUserAgent(const QString& userAgent)
{
    WebClient_P::setDefaultUserAgent(userAgent);
}

QString OsmAnd::WebClient::getUserAgent() const
{
    return _p->getUserAgent();
}

void OsmAnd::WebClient::setUserAgent(const QString& userAgent)
{
    _p->setUserAgent(userAgent);
}

unsigned int OsmAnd::WebClient::getConcurrentRequestsLimit() const
{
    return _p->getConcurrentRequestsLimit();
}

void OsmAnd::WebClient::setConcurrentRequestsLimit(const unsigned int newLimit)
{
    _p->setConcurrentRequestsLimit(newLimit);
}

unsigned int OsmAnd::WebClient::getRetriesLimit() const
{
    return _p->getRetriesLimit();
}

void OsmAnd::WebClient::setRetriesLimit(const unsigned int newLimit)
{
    _p->setRetriesLimit(newLimit);
}

bool OsmAnd::WebClient::getFollowRedirects() const
{
    return _p->getFollowRedirects();
}

void OsmAnd::WebClient::setFollowRedirects(const bool followRedirects)
{
    _p->setFollowRedirects(followRedirects);
}

QByteArray OsmAnd::WebClient::downloadData(
    const QNetworkRequest& networkRequest,
    std::shared_ptr<const RequestResult>* const requestResult /*= nullptr*/,
    const RequestProgressCallbackSignature progressCallback /*= nullptr*/)
{
    return _p->downloadData(networkRequest, requestResult, progressCallback);
}

QByteArray OsmAnd::WebClient::downloadData(
    const QUrl& url,
    std::shared_ptr<const RequestResult>* const requestResult /*= nullptr*/,
    const RequestProgressCallbackSignature progressCallback /*= nullptr*/)
{
    return downloadData(QNetworkRequest(url), requestResult, progressCallback);
}

QString OsmAnd::WebClient::downloadString(
    const QNetworkRequest& networkRequest,
    std::shared_ptr<const RequestResult>* const requestResult /*= nullptr*/,
    const RequestProgressCallbackSignature progressCallback /*= nullptr*/)
{
    return _p->downloadString(networkRequest, requestResult, progressCallback);
}

QString OsmAnd::WebClient::downloadString(
    const QUrl& url,
    std::shared_ptr<const RequestResult>* const requestResult /*= nullptr*/,
    const RequestProgressCallbackSignature progressCallback /*= nullptr*/)
{
    return downloadString(QNetworkRequest(url), requestResult, progressCallback);
}

bool OsmAnd::WebClient::downloadFile(
    const QNetworkRequest& networkRequest,
    const QString& fileName,
    std::shared_ptr<const RequestResult>* const requestResult /*= nullptr*/,
    const RequestProgressCallbackSignature progressCallback /*= nullptr*/)
{
    return _p->downloadFile(networkRequest, fileName, requestResult, progressCallback);
}

bool OsmAnd::WebClient::downloadFile(
    const QUrl& url,
    const QString& fileName,
    std::shared_ptr<const RequestResult>* const requestResult /*= nullptr*/,
    const RequestProgressCallbackSignature progressCallback /*= nullptr*/)
{
    return downloadFile(QNetworkRequest(url), fileName, requestResult, progressCallback);
}

OsmAnd::WebClient::RequestResult::RequestResult(const QNetworkReply* const networkReply)
    : errorCode(networkReply->error())
{
}

OsmAnd::WebClient::RequestResult::~RequestResult()
{
}

bool OsmAnd::WebClient::RequestResult::isSuccessful() const
{
    return (errorCode == QNetworkReply::NoError);
}

OsmAnd::WebClient::HttpRequestResult::HttpRequestResult(const QNetworkReply* const networkReply)
    : RequestResult(networkReply)
    , httpStatusCode(networkReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toUInt())
{
}

OsmAnd::WebClient::HttpRequestResult::~HttpRequestResult()
{
}
