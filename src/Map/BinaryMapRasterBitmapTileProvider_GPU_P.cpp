#include "BinaryMapRasterBitmapTileProvider_GPU_P.h"
#include "BinaryMapRasterBitmapTileProvider_GPU.h"

//#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include <cassert>
#if OSMAND_PERFORMANCE_METRICS
#   include <chrono>
#endif // OSMAND_PERFORMANCE_METRICS

#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>

#include "BinaryMapDataProvider.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "Rasterizer.h"
#include "RasterizerContext.h"
#include "RasterizerEnvironment.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapRasterBitmapTileProvider_GPU_P::BinaryMapRasterBitmapTileProvider_GPU_P(BinaryMapRasterBitmapTileProvider_GPU* owner_)
    : BinaryMapRasterBitmapTileProvider_P(owner_)
    , owner(owner_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider_GPU_P::~BinaryMapRasterBitmapTileProvider_GPU_P()
{
}

bool OsmAnd::BinaryMapRasterBitmapTileProvider_GPU_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<const MapTiledData>& outTiledData,
    const IQueryController* const queryController)
{
    // Obtain offline map data tile
    std::shared_ptr<const MapTiledData > dataTile_;
    owner->dataProvider->obtainData(tileId, zoom, dataTile_);
    if (!dataTile_)
    {
        outTiledData.reset();
        return true;
    }
    const auto dataTile = std::static_pointer_cast<const BinaryMapDataTile>(dataTile_);

#if OSMAND_PERFORMANCE_METRICS
    const auto dataRasterization_Begin = std::chrono::high_resolution_clock::now();
#endif // OSMAND_PERFORMANCE_METRICS

    // Allocate rasterization target
    const std::shared_ptr<SkBitmap> rasterizationSurface(new SkBitmap());
    rasterizationSurface->setConfig(SkBitmap::kARGB_8888_Config, owner->tileSize, owner->tileSize);
    if (!rasterizationSurface->allocPixels())
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for ARGB8888 rasterization surface %dx%d", owner->tileSize, owner->tileSize);
        return false;
    }
    //TODO: SkGpuDevice
    SkBitmapDevice rasterizationTarget(*rasterizationSurface);

    // Create rasterization canvas
    SkCanvas canvas(&rasterizationTarget);

    // Perform actual rendering
    if (!dataTile->nothingToRasterize)
    {
        Rasterizer rasterizer(dataTile->rasterizerContext);
        rasterizer.rasterizeMap(canvas, true, nullptr, queryController);
    }

#if OSMAND_PERFORMANCE_METRICS
    const auto dataRasterization_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataRasterization_Elapsed = dataRasterization_End - dataRasterization_Begin;
    if (!dataTile->nothingToRasterize)
    {
        LogPrintf(LogSeverityLevel::Info,
            "%d map objects in %dx%d@%d: rasterization %fs",
            dataTile->mapObjects.count(), tileId.x, tileId.y, zoom, dataRasterization_Elapsed.count());
    }
    else
    {
        LogPrintf(LogSeverityLevel::Info,
            "%d map objects in %dx%d@%d: nothing to rasterize (%fs)",
            dataTile->mapObjects.count(), tileId.x, tileId.y, zoom, dataRasterization_Elapsed.count());
    }
#endif // OSMAND_PERFORMANCE_METRICS

    // If there is no data to rasterize, tell that this tile is not available
    if (dataTile->nothingToRasterize)
    {
        outTiledData.reset();
        return true;
    }

    // Or supply newly rasterized tile
    const auto newTile = new BinaryMapRasterizedTile(
        dataTile,
        rasterizationSurface,
        AlphaChannelData::NotPresent,
        owner->densityFactor,
        tileId,
        zoom);
    outTiledData.reset(newTile);

    return true;
}
