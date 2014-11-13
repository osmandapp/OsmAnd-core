#include "MapRasterLayerProvider_Software_P.h"
#include "MapRasterLayerProvider_Software.h"

//#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include "ignore_warnings_on_external_includes.h"
#include <SkStream.h>
#include <SkBitmap.h>
#include <SkCanvas.h>
#include <SkBitmapDevice.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>
#include "restore_internal_warnings.h"

#include "MapPrimitivesProvider.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "MapRasterizer.h"
#include "MapPresentationEnvironment.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapRasterLayerProvider_Software_P::MapRasterLayerProvider_Software_P(MapRasterLayerProvider_Software* owner_)
    : MapRasterLayerProvider_P(owner_)
    , owner(owner_)
{
}

OsmAnd::MapRasterLayerProvider_Software_P::~MapRasterLayerProvider_Software_P()
{
}

std::shared_ptr<SkBitmap> OsmAnd::MapRasterLayerProvider_Software_P::rasterize(
    const TileId tileId,
    const ZoomLevel zoom,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& primitivesTile,
    MapRasterLayerProvider_Metrics::Metric_obtainData* const metric_,
    const IQueryController* const queryController)
{
#if OSMAND_PERFORMANCE_METRICS
    MapRasterLayerProvider_Metrics::Metric_obtainData localMetric;
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

    // Allocate rasterization target
    const auto tileSize = owner->getTileSize();
    const std::shared_ptr<SkBitmap> rasterizationSurface(new SkBitmap());
    rasterizationSurface->setConfig(SkBitmap::kARGB_8888_Config, tileSize, tileSize);
    if (!rasterizationSurface->allocPixels())
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to allocate buffer for ARGB8888 rasterization surface %dx%d", tileSize, tileSize);
        return nullptr;
    }
    SkBitmapDevice rasterizationTarget(*rasterizationSurface);

    // Create rasterization canvas
    SkCanvas canvas(&rasterizationTarget);

    // Perform actual rendering
    _mapRasterizer->rasterize(
        primitivesTile->primitivisedArea,
        canvas,
        true,
        nullptr,
        metric ? metric->findOrAddSubmetricOfType<MapRasterizer_Metrics::Metric_rasterize>().get() : nullptr,
        queryController);

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

#if OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS <= 1
    LogPrintf(LogSeverityLevel::Info,
        "%dx%d@%d rasterized on CPU in %fs",
        tileId.x,
        tileId.y,
        zoom,
        totalStopwatch.elapsed());
#else
    LogPrintf(LogSeverityLevel::Info,
        "%dx%d@%d rasterized on CPU in %fs:\n%s",
        tileId.x,
        tileId.y,
        zoom,
        totalStopwatch.elapsed(),
        qPrintable(metric ? metric->toString(QLatin1String("\t - ")) : QLatin1String("(null)")));
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS

    return rasterizationSurface;
}
