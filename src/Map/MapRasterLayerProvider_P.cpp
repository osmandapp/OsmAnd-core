#include "MapRasterLayerProvider_P.h"
#include "MapRasterLayerProvider.h"

//#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_Metrics.h"
#include "MapPrimitiviser.h"
#include "MapRasterizer.h"
#include "Stopwatch.h"

OsmAnd::MapRasterLayerProvider_P::MapRasterLayerProvider_P(MapRasterLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapRasterLayerProvider_P::~MapRasterLayerProvider_P()
{
}

void OsmAnd::MapRasterLayerProvider_P::initialize()
{
    _mapRasterizer.reset(new MapRasterizer(owner->primitivesProvider->primitiviser->environment));
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider_P::getMinZoom() const
{
    return owner->primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider_P::getMaxZoom() const
{
    return owner->primitivesProvider->getMaxZoom();
}

bool OsmAnd::MapRasterLayerProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapRasterLayerProvider::Data>& outTiledData,
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

    // Obtain offline map primitives tile
    std::shared_ptr<MapPrimitivesProvider::Data> primitivesTile;
    owner->primitivesProvider->obtainData(
        tileId,
        zoom,
        primitivesTile,
        metric ? metric->findOrAddSubmetricOfType<MapPrimitivesProvider_Metrics::Metric_obtainData>().get() : nullptr,
        nullptr);
    if (!primitivesTile || primitivesTile->primitivisedObjects->isEmpty())
    {
        outTiledData.reset();

        if (metric)
            metric->elapsedTime += totalStopwatch.elapsed();

        return true;
    }

    // Perform actual rasterization
    const auto bitmap = rasterize(tileId, zoom, primitivesTile, metric, queryController);
    if (!bitmap)
    {
        if (metric)
            metric->elapsedTime += totalStopwatch.elapsed();

        return false;
    }

    // Or supply newly rasterized tile
    outTiledData.reset(new MapRasterLayerProvider::Data(
        tileId,
        zoom,
        AlphaChannelPresence::NotPresent,
        owner->getTileDensityFactor(),
        bitmap,
        primitivesTile,
        new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata)));

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return true;
}

OsmAnd::MapRasterLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::MapRasterLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
