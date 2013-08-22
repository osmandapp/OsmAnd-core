#include "OfflineMapRasterTileProvider_P.h"
#include "OfflineMapRasterTileProvider.h"

#include <assert.h>
#include <chrono>

#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include "OfflineMapDataProvider.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "Rasterizer.h"
#include "RasterizerContext.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::OfflineMapRasterTileProvider_P::OfflineMapRasterTileProvider_P( OfflineMapRasterTileProvider* owner_ )
    : owner(owner_)
    , _taskHostBridge(this)
{
}

OsmAnd::OfflineMapRasterTileProvider_P::~OfflineMapRasterTileProvider_P()
{
}

void OsmAnd::OfflineMapRasterTileProvider_P::obtainTileDeffered( const TileId& tileId, const ZoomLevel& zoom, IMapTileProvider::TileReadyCallback readyCallback )
{
    assert(readyCallback != nullptr);

    std::shared_ptr<TileEntry> tileEntry;
    _tiles.obtainTileEntry(tileEntry, tileId, zoom, true);
    {
        QWriteLocker scopeLock(&tileEntry->stateLock);
        if(tileEntry->state != TileState::Unknown)
            return;

        tileEntry->state = TileState::Requested;
    }

    Concurrent::pools->localStorage->start(new Concurrent::HostedTask(_taskHostBridge,
        [tileId, zoom, readyCallback](const Concurrent::Task* task, QEventLoop& eventLoop)
    {
        const auto pThis = reinterpret_cast<OfflineMapRasterTileProvider_P*>(static_cast<const Concurrent::HostedTask*>(task)->lockedOwner);

        // Get tile entry
        std::shared_ptr<TileEntry> tileEntry;
        pThis->_tiles.obtainTileEntry(tileEntry, tileId, zoom, true);
        {
            QWriteLocker scopeLock(&tileEntry->stateLock);
            if(tileEntry->state != TileState::Requested)
                return;

            tileEntry->state = TileState::Processing;
        }

        // Get bounding box that covers this tile
        const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

        // Obtain OBF data interface
        const auto& dataInterface = pThis->owner->dataProvider->obfsCollection->obtainDataInterface();

        // Get map objects from data proxy
        QList< std::shared_ptr<const Model::MapObject> > mapObjects;
#if defined(_DEBUG) || defined(DEBUG)
        const auto dataRead_Begin = std::chrono::high_resolution_clock::now();
#endif
        dataInterface->obtainMapObjects(&mapObjects, tileBBox31, zoom, nullptr);
#if defined(_DEBUG) || defined(DEBUG)
        const auto dataRead_End = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> dataRead_Elapsed = dataRead_End - dataRead_Begin;
        LogPrintf(LogSeverityLevel::Info, "Read %d map objects from %dx%d@%d in %f seconds", mapObjects.count(), tileId.x, tileId.y, zoom, dataRead_Elapsed.count());
#endif

#if defined(_DEBUG) || defined(DEBUG)
        const auto dataRasterization_Begin = std::chrono::high_resolution_clock::now();
#endif

        // Allocate rasterization target
        auto rasterizationSurface = new SkBitmap();
        rasterizationSurface->setConfig(SkBitmap::kARGB_8888_Config, pThis->owner->tileSize, pThis->owner->tileSize);
        if(!rasterizationSurface->allocPixels())
        {
            delete rasterizationSurface;

            LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for ARGB8888 rasterization surface %dx%d", pThis->owner->tileSize, pThis->owner->tileSize);
            return;
        }
        SkDevice rasterizationTarget(*rasterizationSurface);

        // Create rasterization canvas
        SkCanvas canvas(&rasterizationTarget);

        // Perform actual rendering
        bool nothingToRasterize = false;
        RasterizerContext rasterizerContext(pThis->owner->dataProvider->mapStyle);
        Rasterizer::update(rasterizerContext, tileBBox31, zoom, pThis->owner->tileSize, pThis->owner->displayDensity, &mapObjects, OsmAnd::PointF(), &nothingToRasterize, nullptr);
        Rasterizer::rasterizeMap(rasterizerContext, true, canvas, nullptr);

#if defined(_DEBUG) || defined(DEBUG)
        const auto dataRasterization_End = std::chrono::high_resolution_clock::now();
        const std::chrono::duration<float> dataRasterization_Elapsed = dataRasterization_End - dataRasterization_Begin;
        LogPrintf(LogSeverityLevel::Info, "Rasterized %d map objects from %dx%d@%d in %f seconds", mapObjects.count(), tileId.x, tileId.y, zoom, dataRasterization_Elapsed.count());
#endif

        // If there is no data to rasterize, tell that this tile is not available
        if(nothingToRasterize)
        {
            delete rasterizationSurface;

            std::shared_ptr<IMapTileProvider::Tile> emptyTile;
            readyCallback(tileId, zoom, emptyTile, false);

            pThis->_tiles.removeEntry(tileEntry);

            return;
        }

        // Or supply newly rasterized tile
        std::shared_ptr<OfflineMapRasterTileProvider::Tile> tile(new OfflineMapRasterTileProvider::Tile(rasterizationSurface, IMapBitmapTileProvider::AlphaChannelData::NotPresent));
        readyCallback(tileId, zoom, tile, true);

        pThis->_tiles.removeEntry(tileEntry);
    }));
}

