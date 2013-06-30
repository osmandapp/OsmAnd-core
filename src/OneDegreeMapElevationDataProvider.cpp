#include "OneDegreeMapElevationDataProvider.h"

#include <assert.h>

#include <QtGlobal>
#include <QtCore/qmath.h>

#include "Concurrent.h"
#include "OsmAndUtilities.h"

OsmAnd::OneDegreeMapElevationDataProvider::OneDegreeMapElevationDataProvider(const uint32_t& valuesPerSide)
    : IMapElevationDataProvider(valuesPerSide)
{
}

OsmAnd::OneDegreeMapElevationDataProvider::~OneDegreeMapElevationDataProvider()
{
}

bool OsmAnd::OneDegreeMapElevationDataProvider::obtainTileImmediate( const TileId& tileId, uint32_t zoom, std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    // Elevation data tiles are not available immediately, since none of them are stored in memory.
    // In that case, a callback will be called
    return false;
}

void OsmAnd::OneDegreeMapElevationDataProvider::obtainTileDeffered( const TileId& tileId, uint32_t zoom, TileReadyCallback readyCallback )
{
    return;
    assert(readyCallback != nullptr);

    {
        QMutexLocker scopeLock(&_requestsMutex);
        if(_requestedTileIds[zoom].contains(tileId))
        {
            //LogPrintf(LogSeverityLevel::Debug, "Request for tile %dx%d@%d ignored: already requested\n", tileId.x, tileId.y, zoom);
            return;
        }
        _requestedTileIds[zoom].insert(tileId);
    }
    //
    //Concurrent::instance()->localStoragePool->start(new Concurrent::Task(
    //    [this, tileId, zoom, readyCallback](const Concurrent::Task* task)
    //{
    //    //_processingMutex.lock();

    //    //LogPrintf(LogSeverityLevel::Debug, "Processing order of tile %dx%d@%d : local-lookup\n", tileId.x, tileId.y, zoom);
    //    /*
    //    // Check if we're already in process of downloading this tile, or
    //    // if this tile is in pending-download state
    //    if(_enqueuedTileIdsForDownload[zoom].contains(tileId) || _currentlyDownloadingTileIds[zoom].contains(tileId))
    //    {
    //        //LogPrintf(LogSeverityLevel::Debug, "Ignoring order of tile %dx%d@%d : already in pending-download or downloading state\n", tileId.x, tileId.y, zoom);
    //        _processingMutex.unlock();
    //        return;
    //    }
    //    */

    //    // Obtain degree-range of requested area
    //    const auto leftDegree = qFloor(Utilities::getLongitudeFromTile(zoom, tileId.y + 0));
    //    const auto rightDegree = qCeil(Utilities::getLongitudeFromTile(zoom, tileId.y + 1));
    //    const auto topDegree = qFloor(Utilities::getLatitudeFromTile(zoom, tileId.x + 0));
    //    const auto bottomDegree = qCeil(Utilities::getLatitudeFromTile(zoom, tileId.x + 1));

    //    /*
    //    // Check if file is already in local storage
    //    if(_localCachePath && _localCachePath->exists())
    //    {
    //        const auto& subPath = id + QDir::separator() +
    //            QString::number(zoom) + QDir::separator() +
    //            QString::number(tileId.x) + QDir::separator() +
    //            QString::number(tileId.y) + QString::fromLatin1(".tile");
    //        const auto& fullPath = _localCachePath->filePath(subPath);
    //        QFile tileFile(fullPath);
    //        if(tileFile.exists())
    //        {
    //            // 0-sized tile means there is no data at all
    //            if(tileFile.size() == 0)
    //            {
    //                //LogPrintf(LogSeverityLevel::Debug, "Order processed of tile %dx%d@%d : local-lookup 0 tile\n", tileId.x, tileId.y, zoom);
    //                {
    //                    QMutexLocker scopeLock(&_requestsMutex);
    //                    _requestedTileIds[zoom].remove(tileId);
    //                }
    //                _processingMutex.unlock();

    //                std::shared_ptr<IMapTileProvider::Tile> emptyTile;
    //                readyCallback(tileId, zoom, emptyTile, true);

    //                return;
    //            }

    //            //TODO: Here may be issue that SKIA can not handle opening files on different platforms correctly

    //            // Determine mode
    //            auto skBitmap = new SkBitmap();
    //            SkFILEStream fileStream(fullPath.toStdString().c_str());
    //            if(!SkImageDecoder::DecodeStream(&fileStream, skBitmap, SkBitmap::kNo_Config, SkImageDecoder::kDecodeBounds_Mode))
    //            {
    //                LogPrintf(LogSeverityLevel::Error, "Failed to decode header of tile file '%s'\n", fullPath.toStdString().c_str());
    //                {
    //                    QMutexLocker scopeLock(&_requestsMutex);
    //                    _requestedTileIds[zoom].remove(tileId);
    //                }
    //                _processingMutex.unlock();

    //                delete skBitmap;
    //                std::shared_ptr<IMapTileProvider::Tile> emptyTile;
    //                readyCallback(tileId, zoom, emptyTile, false);
    //                return;
    //            }

    //            const bool force32bit = (skBitmap->getConfig() != SkBitmap::kRGB_565_Config) && (skBitmap->getConfig() != SkBitmap::kARGB_4444_Config);
    //            fileStream.rewind();
    //            if(!SkImageDecoder::DecodeStream(&fileStream, skBitmap, force32bit ? SkBitmap::kARGB_8888_Config : skBitmap->getConfig(), SkImageDecoder::kDecodePixels_Mode))
    //            {
    //                LogPrintf(LogSeverityLevel::Error, "Failed to decode tile file '%s'\n", fullPath.toStdString().c_str());
    //                {
    //                    QMutexLocker scopeLock(&_requestsMutex);
    //                    _requestedTileIds[zoom].remove(tileId);
    //                }
    //                _processingMutex.unlock();

    //                delete skBitmap;
    //                std::shared_ptr<IMapTileProvider::Tile> emptyTile;
    //                readyCallback(tileId, zoom, emptyTile, false);
    //                return;
    //            }

    //            assert(skBitmap->width() == skBitmap->height());
    //            assert(skBitmap->width() == tileDimension);

    //            // Construct tile response
    //            //LogPrintf(LogSeverityLevel::Debug, "Order processed of tile %dx%d@%d : local-lookup\n", tileId.x, tileId.y, zoom);
    //            {
    //                QMutexLocker scopeLock(&_requestsMutex);
    //                _requestedTileIds[zoom].remove(tileId);
    //            }
    //            _processingMutex.unlock();

    //            std::shared_ptr<Tile> tile(new Tile(skBitmap));
    //            readyCallback(tileId, zoom, tile, true);
    //            return;
    //        }
    //    }

    //    // Well, tile is not in local cache, we need to download it
    //    assert(false);
    //    auto tileUrl = urlPattern;
    //    tileUrl
    //        .replace(QString::fromLatin1("${zoom}"), QString::number(zoom))
    //        .replace(QString::fromLatin1("${x}"), QString::number(tileId.x))
    //        .replace(QString::fromLatin1("${y}"), QString::number(tileId.y));
    //    obtainTileDeffered(QUrl(tileUrl), tileId, zoom, readyCallback);*/
    //}));
}

