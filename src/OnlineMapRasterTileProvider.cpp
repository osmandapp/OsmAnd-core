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
    , _currentDownloadsMutex(QMutex::Recursive)
    , _tileDimension(0)
    , _localCacheAccessMutex(QMutex::Recursive)
    , _networkAccessAllowed(true)
    , _downloadQueueMutex(QMutex::Recursive)
    , localCachePath(_localCachePath)
    , networkAccessAllowed(_networkAccessAllowed)
{
}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{
}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const QDir& localCachePath )
{
    QMutexLocker scopeLock(&_localCacheAccessMutex);
    _localCachePath.reset(new QDir(localCachePath));
}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
{
    _networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTile(
    const TileId& tileId, uint32_t zoom,
    std::shared_ptr<SkBitmap>& tileBitmap,
    SkBitmap::Config preferredConfig )
{
    QMutexLocker scopeLock(&_localCacheAccessMutex);

    if(!_localCachePath)
        return false;

    if(!_localCachePath->exists())
        return false;

    const auto& subPath = _id + QDir::separator() +
        QString::number(zoom) + QDir::separator() +
        QString::number(tileId.x) + QDir::separator() +
        QString::number(tileId.y) + QString::fromLatin1(".tile");
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

void OsmAnd::OnlineMapRasterTileProvider::obtainTile( const TileId& tileId, uint32_t zoom, TileReceiverCallback receiverCallback, SkBitmap::Config preferredConfig )
{
    if(!_networkAccessAllowed)
        return;

    auto tileUrl = _urlPattern;
    tileUrl
        .replace(QString::fromLatin1("${zoom}"), QString::number(zoom))
        .replace(QString::fromLatin1("${x}"), QString::number(tileId.x))
        .replace(QString::fromLatin1("${y}"), QString::number(tileId.y));

    obtainTile(QUrl(tileUrl), tileId, zoom, receiverCallback, preferredConfig);
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTile( const QUrl& url, const TileId& tileId, uint32_t zoom, TileReceiverCallback receiverCallback, SkBitmap::Config preferredConfig )
{
    if(!_networkAccessAllowed)
        return;

    {
        QMutexLocker scopeLock(&_currentDownloadsMutex);
        if(_currentDownloads.contains(tileId))
            return;
        if(_currentDownloads.size() == _maxConcurrentDownloads)
        {
            QMutexLocker scopeLock(&_downloadQueueMutex);
            if(_enqueuedTileIds.contains(tileId))
                return;

            TileRequest tileRequest;
            tileRequest.sourceUrl = url;
            tileRequest.tileId = tileId;
            tileRequest.zoom = zoom;
            tileRequest.callback = receiverCallback;
            tileRequest.preferredConfig = preferredConfig;
           
            _tileRequestsQueue.enqueue(tileRequest);
            _enqueuedTileIds.insert(tileId);
        
            return;
        }
        _currentDownloads.insert(tileId);
    }

    QCoreApplication::postEvent(gMainThreadTaskHost.get(), new QMainThreadTaskEvent(
        [this, url, tileId, zoom, receiverCallback, preferredConfig] ()
        {
            LogPrintf(LogSeverityLevel::Info, "Downloading tile %dx%d@%d from %s\n", tileId.x, tileId.y, zoom, url.toString().toStdString().c_str());
            
            QNetworkRequest request;
            request.setUrl(url);
            request.setRawHeader("User-Agent", "OsmAnd Core");

            auto reply = gNetworkAccessManager->get(request);
            QObject::connect(reply, &QNetworkReply::finished,
                [this, tileId, zoom, reply, receiverCallback, preferredConfig] ()
                {
                    auto redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                    if(!redirectUrl.isEmpty())
                    {
                        {
                            QMutexLocker scopeLock(&_currentDownloadsMutex);
                            _currentDownloads.remove(tileId);

                            obtainTile(redirectUrl, tileId, zoom, receiverCallback, preferredConfig);
                        }

                        reply->deleteLater();
                        return;
                    }
                    
                    handleNetworkReply(reply, tileId, zoom, receiverCallback, preferredConfig);
                    reply->deleteLater();

                    {
                        QMutexLocker scopeLock(&_currentDownloadsMutex);
                        _currentDownloads.remove(tileId);
                    }

                    {
                        QMutexLocker scopeLock(&_downloadQueueMutex);
                        
                        LogPrintf(LogSeverityLevel::Info, "Tile downloading queue size %d\n", _tileRequestsQueue.size());
                        while(!_tileRequestsQueue.isEmpty())
                        {
                            {
                                QMutexLocker scopeLock(&_currentDownloadsMutex);
                                if(_currentDownloads.size() == _maxConcurrentDownloads)
                                    break;
                            }

                            const auto& request = _tileRequestsQueue.dequeue();
                            _enqueuedTileIds.remove(request.tileId);
                            obtainTile(request.sourceUrl, request.tileId, request.zoom, request.callback, request.preferredConfig);
                        }
                    }
                }
            );
        }
    ));
}

void OsmAnd::OnlineMapRasterTileProvider::handleNetworkReply( QNetworkReply* reply, const TileId& tileId, uint32_t zoom, TileReceiverCallback receiverCallback, SkBitmap::Config preferredConfig )
{
    auto error = reply->error();
    if(error != QNetworkReply::NetworkError::NoError)
    {
        const auto httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

        LogPrintf(LogSeverityLevel::Warning, "Failed to download tile from %s (HTTP status %d)\n", reply->request().url().toString().toStdString().c_str(), httpStatus);

        // 404 means that this tile does not exist, so create a zero file
        if(httpStatus == 404)
        {
            QMutexLocker scopeLock(&_localCacheAccessMutex);

            if(_localCachePath && _localCachePath->exists())
            {
                // Save to a file
                const auto& subPath = _id + QDir::separator() +
                    QString::number(zoom) + QDir::separator() +
                    QString::number(tileId.x) + QDir::separator();
                _localCachePath->mkpath(subPath);
                const auto& fullPath = _localCachePath->filePath(subPath + QString::number(tileId.y) + QString::fromLatin1(".tile"));
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

    {
        QMutexLocker scopeLock(&_localCacheAccessMutex);

        if(_localCachePath && _localCachePath->exists())
        {
            // Save to a file
            const auto& subPath = _id + QDir::separator() +
                QString::number(zoom) + QDir::separator() +
                QString::number(tileId.x) + QDir::separator();
            _localCachePath->mkpath(subPath);
            const auto& fullPath = _localCachePath->filePath(subPath + QString::number(tileId.y) + QString::fromLatin1(".tile"));
            QFile tileFile(fullPath);
            if(tileFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
            {
                tileFile.write(data);
                tileFile.close();
            }
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
