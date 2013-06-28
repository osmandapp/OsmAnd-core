#include "OnlineMapRasterTileProvider.h"

#include <assert.h>

#include "OsmAndCore_private.h"
#include "QMainThreadTaskEvent.h"
#include "OsmAndLogging.h"
#include "Concurrent.h"

#include <QCoreApplication>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include <SkStream.h>
#include <SkImageDecoder.h>

OsmAnd::OnlineMapRasterTileProvider::OnlineMapRasterTileProvider(const QString& id_, const QString& urlPattern_, uint32_t maxZoom_ /*= 31*/, uint32_t minZoom_ /*= 0*/, uint32_t maxConcurrentDownloads_ /*= 1*/, uint32_t tileDimension_ /*= 256*/)
    : _processingMutex(QMutex::Recursive)
    , _localCachePath(new QDir(QDir::current()))
    , _networkAccessAllowed(true)
    , _requestsMutex(QMutex::Recursive)
    , localCachePath(_localCachePath)
    , networkAccessAllowed(_networkAccessAllowed)
    , id(id_)
    , urlPattern(urlPattern_)
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
    , maxConcurrentDownloads(maxConcurrentDownloads_)
    , tileDimension(tileDimension_)
{
}

OsmAnd::OnlineMapRasterTileProvider::~OnlineMapRasterTileProvider()
{
}

void OsmAnd::OnlineMapRasterTileProvider::setLocalCachePath( const QDir& localCachePath )
{
    QMutexLocker scopeLock(&_processingMutex);
    _localCachePath.reset(new QDir(localCachePath));
}

void OsmAnd::OnlineMapRasterTileProvider::setNetworkAccessPermission( bool allowed )
{
    _networkAccessAllowed = allowed;
}

bool OsmAnd::OnlineMapRasterTileProvider::obtainTileImmediate( const TileId& tileId, uint32_t zoom, std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    // Raster tiles are not available immediately, since none of them are stored in memory unless they are just
    // downloaded. In that case, a callback will be called
    return false;
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTileDeffered( const TileId& tileId, uint32_t zoom, TileReadyCallback readyCallback )
{
    assert(readyCallback != nullptr);

    {
        QMutexLocker scopeLock(&_requestsMutex);
        if(_requestedTileIds[zoom].contains(tileId))
        {
            LogPrintf(LogSeverityLevel::Debug, "Request for tile %dx%d@%d ignored: already requested\n", tileId.x, tileId.y, zoom);
            return;
        }
        _requestedTileIds[zoom].insert(tileId);
    }

    Concurrent::instance()->localStoragePool->start(new Concurrent::Task(
        [this, tileId, zoom, readyCallback](const Concurrent::Task* task)
        {
            _processingMutex.lock();

            LogPrintf(LogSeverityLevel::Debug, "Processing order of tile %dx%d@%d : local-lookup\n", tileId.x, tileId.y, zoom);

            // Check if we're already in process of downloading this tile, or
            // if this tile is in pending-download state
            if(_enqueuedTileIdsForDownload[zoom].contains(tileId) || _currentlyDownloadingTileIds[zoom].contains(tileId))
            {
                LogPrintf(LogSeverityLevel::Debug, "Ignoring order of tile %dx%d@%d : already in pending-download or downloading state\n", tileId.x, tileId.y, zoom);
                _processingMutex.unlock();
                return;
            }

            // Check if file is already in local storage
            if(_localCachePath && _localCachePath->exists())
            {
                const auto& subPath = id + QDir::separator() +
                    QString::number(zoom) + QDir::separator() +
                    QString::number(tileId.x) + QDir::separator() +
                    QString::number(tileId.y) + QString::fromLatin1(".tile");
                const auto& fullPath = _localCachePath->filePath(subPath);
                QFile tileFile(fullPath);
                if(tileFile.exists())
                {
                    // 0-sized tile means there is no data at all
                    if(tileFile.size() == 0)
                    {
                        LogPrintf(LogSeverityLevel::Debug, "Order processed of tile %dx%d@%d : local-lookup 0 tile\n", tileId.x, tileId.y, zoom);
                        {
                            QMutexLocker scopeLock(&_requestsMutex);
                            _requestedTileIds[zoom].remove(tileId);
                        }
                        _processingMutex.unlock();

                        std::shared_ptr<IMapTileProvider::Tile> emptyTile;
                        readyCallback(tileId, zoom, emptyTile, true);

                        return;
                    }

                    //TODO: Here may be issue that SKIA can not handle opening files on different platforms correctly

                    // Determine mode
                    auto skBitmap = new SkBitmap();
                    SkFILEStream fileStream(fullPath.toStdString().c_str());
                    if(!SkImageDecoder::DecodeStream(&fileStream, skBitmap, SkBitmap::kNo_Config, SkImageDecoder::kDecodeBounds_Mode))
                    {
                        LogPrintf(LogSeverityLevel::Error, "Failed to decode header of tile file '%s'\n", fullPath.toStdString().c_str());
                        {
                            QMutexLocker scopeLock(&_requestsMutex);
                            _requestedTileIds[zoom].remove(tileId);
                        }
                        _processingMutex.unlock();

                        delete skBitmap;
                        std::shared_ptr<IMapTileProvider::Tile> emptyTile;
                        readyCallback(tileId, zoom, emptyTile, false);
                        return;
                    }

                    const bool force32bit = (skBitmap->getConfig() != SkBitmap::kRGB_565_Config) && (skBitmap->getConfig() != SkBitmap::kARGB_4444_Config);
                    fileStream.rewind();
                    if(!SkImageDecoder::DecodeStream(&fileStream, skBitmap, force32bit ? SkBitmap::kARGB_8888_Config : skBitmap->getConfig(), SkImageDecoder::kDecodePixels_Mode))
                    {
                        LogPrintf(LogSeverityLevel::Error, "Failed to decode tile file '%s'\n", fullPath.toStdString().c_str());
                        {
                            QMutexLocker scopeLock(&_requestsMutex);
                            _requestedTileIds[zoom].remove(tileId);
                        }
                        _processingMutex.unlock();

                        delete skBitmap;
                        std::shared_ptr<IMapTileProvider::Tile> emptyTile;
                        readyCallback(tileId, zoom, emptyTile, false);
                        return;
                    }

                    assert(skBitmap->width() == skBitmap->height());
                    assert(skBitmap->width() == tileDimension);

                    // Construct tile response
                    LogPrintf(LogSeverityLevel::Debug, "Order processed of tile %dx%d@%d : local-lookup\n", tileId.x, tileId.y, zoom);
                    {
                        QMutexLocker scopeLock(&_requestsMutex);
                        _requestedTileIds[zoom].remove(tileId);
                    }
                    _processingMutex.unlock();
                    
                    std::shared_ptr<Tile> tile(new Tile(skBitmap));
                    readyCallback(tileId, zoom, tile, true);
                    return;
                }
            }
        
            // Well, tile is not in local cache, we need to download it
            assert(false);
            auto tileUrl = urlPattern;
            tileUrl
                .replace(QString::fromLatin1("${zoom}"), QString::number(zoom))
                .replace(QString::fromLatin1("${x}"), QString::number(tileId.x))
                .replace(QString::fromLatin1("${y}"), QString::number(tileId.y));
            obtainTileDeffered(QUrl(tileUrl), tileId, zoom, readyCallback);
        }));

    /*
    QMutexLocker scopeLock(&_localCacheAccessMutex);

    if(!_localCachePath)
        return false;

    if(!_localCachePath->exists())
        return false;

    

    return true;
    */
    /*if(!_networkAccessAllowed)
        return;

    

    obtainTileDeffered(QUrl(tileUrl), tileId, zoom, receiverCallback, preferredConfig);*/
}

void OsmAnd::OnlineMapRasterTileProvider::obtainTileDeffered( const QUrl& url, const TileId& tileId, uint32_t zoom, TileReadyCallback readyCallback )
{
    QMutexLocker scopeLock(&_processingMutex);

    if(!_networkAccessAllowed)
        return;
    
    if(_currentlyDownloadingTileIds.size() == maxConcurrentDownloads)
    {
        // All slots are taken, enqueue
        TileRequest tileRequest;
        tileRequest.sourceUrl = url;
        tileRequest.tileId = tileId;
        tileRequest.zoom = zoom;
        tileRequest.callback = readyCallback;

        _tileDownloadRequestsQueue.enqueue(tileRequest);
        _enqueuedTileIdsForDownload[zoom].insert(tileId);

        return;
    }
    _currentlyDownloadingTileIds[zoom].insert(tileId);
    Concurrent::instance()->networkPool->start(new Concurrent::Task(
        [this, tileId, zoom, readyCallback](const Concurrent::Task* task)
    {

    }));
    /*
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

                            obtainTileDeffered(redirectUrl, tileId, zoom, receiverCallback, preferredConfig);
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
                            obtainTileDeffered(request.sourceUrl, request.tileId, request.zoom, request.callback, request.preferredConfig);
                        }
                    }
                }
            );
        }
    ));*/
}
/*
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
            assert(tileBitmap->width() == _tileDimension);
            receiverCallback(tileId, zoom, tileBitmap);
        }
    }
}
*/
float OsmAnd::OnlineMapRasterTileProvider::getTileDensity() const
{
    // Online tile providers do not have any idea about our tile density
    return 1.0f;
}

uint32_t OsmAnd::OnlineMapRasterTileProvider::getTileSize() const
{
    return tileDimension;
}

OsmAnd::OnlineMapRasterTileProvider::Tile::Tile( SkBitmap* bitmap )
    : IMapBitmapTileProvider::Tile(bitmap->getPixels(), bitmap->rowBytes(), bitmap->width(), bitmap->height(),
        bitmap->getConfig() == SkBitmap::kARGB_8888_Config ? IMapBitmapTileProvider::ARGB_8888 : (bitmap->getConfig() == SkBitmap::kARGB_4444_Config ? IMapBitmapTileProvider::ARGB_4444 : IMapBitmapTileProvider::RGB_565 ) )
    , _skBitmap(bitmap)
{
}

OsmAnd::OnlineMapRasterTileProvider::Tile::~Tile()
{
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
