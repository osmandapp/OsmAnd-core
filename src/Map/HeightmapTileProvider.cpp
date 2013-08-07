#include "HeightmapTileProvider.h"

#include <assert.h>

#include "Concurrent.h"
#include "Logging.h"

#include <gdal.h>
#include <gdal_priv.h>
#include <cpl_vsi.h>

const QString OsmAnd::HeightmapTileProvider::defaultIndexFilename("heightmap.index");

OsmAnd::HeightmapTileProvider::HeightmapTileProvider( const QDir& dataPath, const QString& indexFilepath/* = QString()*/ )
    : _tileDb(dataPath, indexFilepath)
    , _taskHostBridge(this)
    , tileDb(_tileDb)
{
}

OsmAnd::HeightmapTileProvider::~HeightmapTileProvider()
{
    _taskHostBridge.onOwnerIsBeingDestructed();
}

void OsmAnd::HeightmapTileProvider::rebuildTileDbIndex()
{
    _tileDb.rebuildIndex();
}

uint32_t OsmAnd::HeightmapTileProvider::getTileSize() const
{
    return 32;
}

bool OsmAnd::HeightmapTileProvider::obtainTileImmediate( const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    // Heightmap tiles are not available immediately, since none of them are stored in memory unless they are just
    // downloaded. In that case, a callback will be called
    return false;
}

void OsmAnd::HeightmapTileProvider::obtainTileDeffered( const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback )
{
    assert(readyCallback != nullptr);

    {
        QMutexLocker scopeLock(&_requestsMutex);
        if(_requestedTileIds[static_cast<unsigned>(zoom)].contains(tileId))
            return;

        _requestedTileIds[zoom].insert(tileId);
    }

    Concurrent::pools->localStorage->start(new Concurrent::HostedTask(_taskHostBridge,
        [tileId, zoom, readyCallback](const Concurrent::Task* task, QEventLoop& eventLoop)
        {
            const auto pThis = reinterpret_cast<HeightmapTileProvider*>(static_cast<const Concurrent::HostedTask*>(task)->lockedOwner);

            pThis->_processingMutex.lock();

            // Obtain raw data from DB
            QByteArray data;
            bool ok = pThis->_tileDb.obtainTileData(tileId, zoom, data);
            if(!ok || data.length() == 0)
            {
                {
                    QMutexLocker scopeLock(&pThis->_requestsMutex);
                    pThis->_requestedTileIds[zoom].remove(tileId);
                }
                pThis->_processingMutex.unlock();

                // There was no data at all, to avoid further requests, mark this tile as empty
                std::shared_ptr<IMapTileProvider::Tile> emptyTile;
                readyCallback(tileId, zoom, emptyTile, true);

                return;
            }

            const auto tileSize = pThis->getTileSize();

            // We have the data, use GDAL to decode this GeoTIFF
            bool success = false;
            std::shared_ptr<IMapTileProvider::Tile> tile;
            QString vmemFilename;
            vmemFilename.sprintf("/vsimem/heightmapTile@%p", data.data());
            VSIFileFromMemBuffer(vmemFilename.toStdString().c_str(), reinterpret_cast<GByte*>(data.data()), data.length(), FALSE);
            auto dataset = reinterpret_cast<GDALDataset*>(GDALOpen(vmemFilename.toStdString().c_str(), GA_ReadOnly));
            if(dataset != nullptr)
            {
                bool bad = false;
                bad = bad || dataset->GetRasterCount() != 1;
                bad = bad || dataset->GetRasterXSize() != tileSize;
                bad = bad || dataset->GetRasterYSize() != tileSize;
                if(bad)
                {
                    if(dataset->GetRasterCount() != 1)
                        LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has %d bands instead of 1", tileId.x, tileId.y, zoom, dataset->GetRasterCount());
                    if(dataset->GetRasterXSize() != tileSize || dataset->GetRasterYSize() != tileSize)
                    {
                        LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has %dx%x size instead of %d", tileId.x, tileId.y, zoom,
                            dataset->GetRasterXSize(), dataset->GetRasterYSize(), tileSize);
                    }
                }
                else
                {
                    auto band = dataset->GetRasterBand(1);

                    bad = bad || band->GetColorTable() != nullptr;
                    bad = bad || band->GetRasterDataType() != GDT_Int16;

                    if(bad)
                    {
                        if(band->GetColorTable() != nullptr)
                            LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has color table", tileId.x, tileId.y, zoom);
                        if(band->GetRasterDataType() != GDT_Int16)
                            LogPrintf(LogSeverityLevel::Error, "Height tile %dx%d@%d has %s data type in band 1", tileId.x, tileId.y, zoom, GDALGetDataTypeName(band->GetRasterDataType()));
                    }
                    else
                    {
                        auto buffer = new float[tileSize*tileSize];

                        auto res = dataset->RasterIO(GF_Read, 0, 0, tileSize, tileSize, buffer, tileSize, tileSize, GDT_Float32, 1, nullptr, 0, 0, 0);
                        if(res != CE_None)
                        {
                            delete[] buffer;
                            LogPrintf(LogSeverityLevel::Error, "Failed to decode height tile %dx%d@%d: %s", tileId.x, tileId.y, zoom, CPLGetLastErrorMsg());
                        }
                        else
                        {
                            tile.reset(new Tile(buffer, tileSize));
                            success = true;
                        }
                    }
                }
                
                GDALClose(dataset);
            }
            VSIUnlink(vmemFilename.toStdString().c_str());

            // Construct tile response
            {
                QMutexLocker scopeLock(&pThis->_requestsMutex);
                pThis->_requestedTileIds[zoom].remove(tileId);
            }
            pThis->_processingMutex.unlock();

            readyCallback(tileId, zoom, tile, success);
        }));
}

OsmAnd::HeightmapTileProvider::Tile::Tile( const float* buffer, uint32_t tileSize )
    : IMapElevationDataProvider::Tile(buffer, tileSize * sizeof(float), tileSize)
    , _buffer(buffer)
{
}

OsmAnd::HeightmapTileProvider::Tile::~Tile()
{
    delete[] _buffer;
}
