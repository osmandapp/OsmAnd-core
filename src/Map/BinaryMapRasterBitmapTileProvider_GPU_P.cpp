#include "BinaryMapRasterBitmapTileProvider_GPU_P.h"
#include "BinaryMapRasterBitmapTileProvider_GPU.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>
#include "restore_internal_warnings.h"

#include "BinaryMapPrimitivesProvider.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"
#include "Stopwatch.h"
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
    std::shared_ptr<MapTiledData>& outTiledData,
    BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData* const metric_,
    const IQueryController* const queryController)
{
#if OSMAND_PERFORMANCE_METRICS
    BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData localMetric;
    const auto metric = metric_ ? metric_ : &localMetric;
#else
    const auto metric = metric_;
#endif

    const Stopwatch totalStopwatch(
#if OSMAND_PERFORMANCE_METRICS
        true
#else
        metric != nullptr
#endif // OSMAND_PERFORMANCE_METRICS
        );

    // Obtain offline map primitives tile
    std::shared_ptr<MapTiledData> primitivesTile_;
    owner->primitivesProvider->obtainData(tileId, zoom, primitivesTile_);
    if (!primitivesTile_)
    {
        outTiledData.reset();
        return true;
    }
    const auto primitivesTile = std::static_pointer_cast<BinaryMapPrimitivesTile>(primitivesTile_);

    //TODO: SkGpuDevice
    // Allocate rasterization target
    const auto tileSize = owner->getTileSize();
    const std::shared_ptr<SkBitmap> rasterizationSurface(new SkBitmap());
    rasterizationSurface->setConfig(SkBitmap::kARGB_8888_Config, tileSize, tileSize);
    if (!rasterizationSurface->allocPixels())
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for ARGB8888 rasterization surface %dx%d", tileSize, tileSize);
        return false;
    }
    SkBitmapDevice rasterizationTarget(*rasterizationSurface);

    // Create rasterization canvas
    SkCanvas canvas(&rasterizationTarget);

    // Perform actual rendering
    if (!primitivesTile->primitivisedArea->isEmpty())
    {
        _mapRasterizer->rasterize(
            primitivesTile->primitivisedArea,
            canvas,
            true,
            nullptr,
            metric ? &metric->rasterizeMetric : nullptr,
            queryController);
    }
    else
    {
        // If there is no data to rasterize, tell that this tile is not available
        outTiledData.reset();
        return true;
    }

    // Or supply newly rasterized tile
    outTiledData.reset(new BinaryMapRasterizedTile(
        primitivesTile,
        rasterizationSurface,
        AlphaChannelData::NotPresent,
        owner->getTileDensityFactor(),
        tileId,
        zoom));

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

#if OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS <= 1
    LogPrintf(LogSeverityLevel::Info,
        "%dx%d@%d rasterized on GPU in %fs",
        tileId.x,
        tileId.y,
        zoom,
        totalStopwatch.elapsed());
#else
    LogPrintf(LogSeverityLevel::Info,
        "%dx%d@%d rasterized on GPU in %fs:\n%s",
        tileId.x,
        tileId.y,
        zoom,
        totalStopwatch.elapsed(),
        qPrintable(metric ? metric->toString(QLatin1String("\t - ")) : QLatin1String("(null)")));
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS

    return true;
}
