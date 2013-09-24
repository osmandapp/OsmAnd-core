#include "OfflineMapRasterTileProvider_GPU_P.h"
#include "OfflineMapRasterTileProvider_GPU.h"

#include <cassert>
#include <chrono>

#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include "OfflineMapDataProvider.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "Rasterizer.h"
#include "RasterizerContext.h"
#include "RasterizerEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::OfflineMapRasterTileProvider_GPU_P::OfflineMapRasterTileProvider_GPU_P( OfflineMapRasterTileProvider_GPU* owner_ )
    : owner(owner_)
    , _taskHostBridge(this)
{
}

OsmAnd::OfflineMapRasterTileProvider_GPU_P::~OfflineMapRasterTileProvider_GPU_P()
{
}

bool OsmAnd::OfflineMapRasterTileProvider_GPU_P::obtainTile(const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<MapTile>& outTile)
{
    // Get bounding box that covers this tile
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Obtain OBF data interface
    const auto& dataInterface = owner->dataProvider->obfsCollection->obtainDataInterface();

    // Get map objects from data proxy
    QList< std::shared_ptr<const Model::MapObject> > mapObjects;
#if defined(_DEBUG) || defined(DEBUG)
    const auto dataRead_Begin = std::chrono::high_resolution_clock::now();
#endif
    bool basemapAvailable;
    MapFoundationType tileFoundation;
    dataInterface->obtainBasemapPresenceFlag(basemapAvailable, nullptr);
    dataInterface->obtainMapObjects(&mapObjects, &tileFoundation, tileBBox31, zoom, nullptr);
#if defined(_DEBUG) || defined(DEBUG)
    const auto dataRead_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataRead_Elapsed = dataRead_End - dataRead_Begin;
#endif

#if defined(_DEBUG) || defined(DEBUG)
    const auto dataRasterization_Begin = std::chrono::high_resolution_clock::now();
#endif

    //TODO: SkGpuDevice
    // Allocate rasterization target
    auto rasterizationSurface = new SkBitmap();
    rasterizationSurface->setConfig(SkBitmap::kARGB_8888_Config, owner->tileSize, owner->tileSize);
    if(!rasterizationSurface->allocPixels())
    {
        delete rasterizationSurface;

        LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for ARGB8888 rasterization surface %dx%d", owner->tileSize, owner->tileSize);
        return false;
    }
    SkBitmapDevice rasterizationTarget(*rasterizationSurface);

    // Create rasterization canvas
    SkCanvas canvas(&rasterizationTarget);

    // Perform actual rendering
    bool nothingToRasterize = false;
    RasterizerEnvironment rasterizerEnv(owner->dataProvider->mapStyle, basemapAvailable, owner->displayDensity);
    RasterizerContext rasterizerContext;
    Rasterizer::prepareContext(rasterizerEnv, rasterizerContext, tileBBox31, zoom, owner->tileSize, tileFoundation, mapObjects, OsmAnd::PointF(), &nothingToRasterize, nullptr);
    if(!nothingToRasterize)
        Rasterizer::rasterizeMap(rasterizerEnv, rasterizerContext, true, canvas, nullptr);

#if defined(_DEBUG) || defined(DEBUG)
    const auto dataRasterization_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataRasterization_Elapsed = dataRasterization_End - dataRasterization_Begin;
    if(!nothingToRasterize)
    {
        LogPrintf(LogSeverityLevel::Info,
            "%d map objects from %dx%d@%d: reading %fs, rasterization %fs",
            mapObjects.count(), tileId.x, tileId.y, zoom, dataRead_Elapsed.count(), dataRasterization_Elapsed.count());
    }
    else
    {
        LogPrintf(LogSeverityLevel::Info,
            "%d map objects from %dx%d@%d: reading %fs, nothing to rasterize (%fs)",
            mapObjects.count(), tileId.x, tileId.y, zoom, dataRead_Elapsed.count(), dataRasterization_Elapsed.count());
    }
#endif

    // If there is no data to rasterize, tell that this tile is not available
    if(nothingToRasterize)
    {
        delete rasterizationSurface;

        outTile.reset();
        return true;
    }

    // Or supply newly rasterized tile
    auto tile = new MapBitmapTile(rasterizationSurface, MapBitmapTile::AlphaChannelData::NotPresent);
    outTile.reset(tile);
    return true;
}
