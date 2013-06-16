#include "OnlineMapRasterTileProvider.h"

#include <assert.h>

#include "OsmAndCore_private.h"
#include "QMainThreadTaskEvent.h"
#include "OsmAndLogging.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <SkImageDecoder.h>

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(const QString& id_, const QString& urlPattern_, uint32_t maxZoom_ /*= 31*/, uint32_t minZoom_ /*= 0*/, uint32_t maxConcurrentDownloads_ /*= 1*/)
    : _id(id_)
    , _urlPattern(urlPattern_)
    , _minZoom(minZoom_)
    , _maxZoom(maxZoom_)
    , _maxConcurrentDownloads(maxConcurrentDownloads_)
    , _concurrentDownloadsCounter(0)
    , _tileDimension(0)
    , _networkAccessAllowed(true)
    , localCachePath(_localCachePath)
    , networkAccessAllowed(_networkAccessAllowed)
{
}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{
}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const QDir& localCachePath )
{
    _localCachePath.reset(new QDir(localCachePath));
}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
{
    _networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTile(
    const uint64_t& tileId, uint32_t zoom,
    std::shared_ptr<SkBitmap>& tileBitmap,
    SkBitmap::Config preferredConfig )
{
    if(!_localCachePath)
        return false;

    if(!_localCachePath->exists())
        return false;

    int32_t xZ = static_cast<int32_t>(tileId >> 32);
    int32_t yZ = static_cast<int32_t>(tileId & 0xFFFFFFFF);
    const auto& subPath = _id + QDir::separator() +
        QString::number(zoom) + QDir::separator() +
        QString::number(xZ) + QDir::separator() +
        QString::number(yZ) + QString::fromLatin1(".tile");
    const auto& fullPath = _localCachePath->filePath(subPath);
    QFile tileFile(fullPath);
    if(!tileFile.exists())
        return false;

    // 0-sized tile means there is no data at all
    if(tileFile.size() == 0)
    {
        tileBitmap.reset();
        return true;
    }
    
    tileBitmap.reset(new SkBitmap());
    if(!SkImageDecoder::DecodeFile(fullPath.toStdString().c_str(), tileBitmap.get(), preferredConfig, SkImageDecoder::kDecodePixels_Mode))
    {
        tileBitmap.reset();
        return false;
    }

    assert(tileBitmap->width() == tileBitmap->height());
    if(_tileDimension == 0)
        _tileDimension = tileBitmap->width();
    assert(tileBitmap->width() == _tileDimension);

    return true;
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTile( const uint64_t& tileId, uint32_t zoom, TileReceiverCallback receiverCallback, SkBitmap::Config preferredConfig )
{
    if(!_networkAccessAllowed)
        return;

    int32_t xZ = static_cast<int32_t>(tileId >> 32);
    int32_t yZ = static_cast<int32_t>(tileId & 0xFFFFFFFF);
    auto tileUrl = _urlPattern;
    tileUrl
        .replace(QString::fromLatin1("${zoom}"), QString::number(zoom))
        .replace(QString::fromLatin1("${x}"), QString::number(xZ))
        .replace(QString::fromLatin1("${y}"), QString::number(yZ));

    obtainTile(QUrl(tileUrl), tileId, zoom, receiverCallback, preferredConfig);
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTile( const QUrl& url, const uint64_t& tileId, uint32_t zoom, TileReceiverCallback receiverCallback, SkBitmap::Config preferredConfig )
{
    if(!_networkAccessAllowed)
        return;

    {
        QMutexLocker scopeLock(&_downloadsMutex);
        if(_maxConcurrentDownloads != 0 && _concurrentDownloadsCounter == _maxConcurrentDownloads)
        {
            TileRequest tileRequest;
            tileRequest.sourceUrl = url;
            tileRequest.tileId = tileId;
            tileRequest.zoom = zoom;
            tileRequest.callback = receiverCallback;
            tileRequest.preferredConfig = preferredConfig;
            _tileRequestsQueue.enqueue(tileRequest);
            return;
        }
        _concurrentDownloadsCounter++;
    }

    QCoreApplication::postEvent(gMainThreadTaskHost.get(), new QMainThreadTaskEvent(
        [this, url, tileId, zoom, receiverCallback, preferredConfig] ()
        {
            int32_t xZ = static_cast<int32_t>(tileId >> 32);
            int32_t yZ = static_cast<int32_t>(tileId & 0xFFFFFFFF);
            LogPrintf(LogSeverityLevel::Info, "Downloading tile %dx%d@%d from %s\n", xZ, yZ, zoom, url.toString().toStdString().c_str());
            
            QNetworkRequest request;
            request.setUrl(url);
            request.setRawHeader("User-Agent", "OsmAnd Core");

            auto reply = gNetworkAccessManager->get(request);
            QObject::connect(reply, &QNetworkReply::finished,
                [this, tileId, zoom, reply, receiverCallback, preferredConfig] ()
                {
                    {
                        QMutexLocker scopeLock(&_downloadsMutex);
                        _concurrentDownloadsCounter--;
                    }

                    auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                    if(!redirectUrl.isEmpty())
                    {
                        obtainTile(redirectUrl, tileId, zoom, receiverCallback, preferredConfig);

                        reply->deleteLater();
                        return;
                    }
                    
                    handleNetworkReply(reply, tileId, zoom, receiverCallback, preferredConfig);
                    reply->deleteLater();

                    if(!_tileRequestsQueue.isEmpty())
                    {
                        const auto& request = _tileRequestsQueue.dequeue();
                        obtainTile(request.sourceUrl, request.tileId, request.zoom, request.callback, request.preferredConfig);
                    }
                }
            );
        }
    ));
}

void OsmAnd::OnlineMapRasterTileProvider::handleNetworkReply( QNetworkReply* reply, const uint64_t& tileId, uint32_t zoom, TileReceiverCallback receiverCallback, SkBitmap::Config preferredConfig )
{
    int32_t xZ = static_cast<int32_t>(tileId >> 32);
    int32_t yZ = static_cast<int32_t>(tileId & 0xFFFFFFFF);

    auto error = reply->error();
    if(error != QNetworkReply::NetworkError::NoError)
    {
        const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        LogPrintf(LogSeverityLevel::Warning, "Failed to download tile from %s (HTTP status %d)\n", reply->request().url().toString().toStdString().c_str(), httpStatus);

        // 404 means that this tile does not exist, so create a zero file
        if(httpStatus == 404)
        {
            if(_localCachePath && _localCachePath->exists())
            {
                // Save to a file
                const auto& subPath = _id + QDir::separator() +
                    QString::number(zoom) + QDir::separator() +
                    QString::number(xZ) + QDir::separator();
                _localCachePath->mkpath(subPath);
                const auto& fullPath = _localCachePath->filePath(subPath + QString::number(yZ) + QString::fromLatin1(".tile"));
                QFile tileFile(fullPath);
                if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
                {
                    tileFile.close();
                }
            }
        }
        
        reply->deleteLater();
        return;
    }

    LogPrintf(LogSeverityLevel::Warning, "Downloaded tile from %s\n", reply->request().url().toString().toStdString().c_str());
    const auto& data = reply->readAll();

    if(_localCachePath && _localCachePath->exists())
    {
        // Save to a file
        const auto& subPath = _id + QDir::separator() +
            QString::number(zoom) + QDir::separator() +
            QString::number(xZ) + QDir::separator();
        _localCachePath->mkpath(subPath);
        const auto& fullPath = _localCachePath->filePath(subPath + QString::number(yZ) + QString::fromLatin1(".tile"));
        QFile tileFile(fullPath);
        if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            tileFile.write(data);
            tileFile.close();
        }
    }

    // Decode in-memory
    if(receiverCallback)
    {
        std::shared_ptr<SkBitmap> tileBitmap(new SkBitmap());
        if(SkImageDecoder::DecodeMemory(data.data(), data.size(), tileBitmap.get(), preferredConfig, SkImageDecoder::kDecodePixels_Mode))
        {
            assert(tileBitmap->width() == tileBitmap->height());
            if(_tileDimension == 0)
                _tileDimension = tileBitmap->width();
            assert(tileBitmap->width() == _tileDimension);
            receiverCallback(tileId, zoom, tileBitmap);
        }
    }
}

uint32_t OsmAnd::OnlineMapRasterTileProvider::getTileDimension() const
{
    return _tileDimension;
}

std::shared_ptr<OsmAnd::IMapTileProvider> OsmAnd::OnlineMapRasterTileProvider::createMapnikProvider()
{
    auto provider = new OsmAnd::OnlineMapRasterTileProvider(
        "mapnik", "http://mapnik.osmand.net/${zoom}/${x}/${y}.png", 0, 18, 2);
    return std::shared_ptr<OsmAnd::IMapTileProvider>(static_cast<OsmAnd::IMapTileProvider*>(provider));
}

std::shared_ptr<OsmAnd::IMapTileProvider> OsmAnd::OnlineMapRasterTileProvider::createCycleMapProvider()
{
    auto provider = new OsmAnd::OnlineMapRasterTileProvider(
        "cyclemap", "http://b.tile.opencyclemap.org/cycle/${zoom}/${x}/${y}.png", 0, 18, 2);
    return std::shared_ptr<OsmAnd::IMapTileProvider>(static_cast<OsmAnd::IMapTileProvider*>(provider));
}
