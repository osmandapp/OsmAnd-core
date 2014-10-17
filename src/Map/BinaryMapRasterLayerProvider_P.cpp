#include "BinaryMapRasterLayerProvider_P.h"
#include "BinaryMapRasterLayerProvider.h"

//#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include "BinaryMapPrimitivesProvider.h"
#include "Primitiviser.h"
#include "MapRasterizer.h"

OsmAnd::BinaryMapRasterLayerProvider_P::BinaryMapRasterLayerProvider_P(BinaryMapRasterLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapRasterLayerProvider_P::~BinaryMapRasterLayerProvider_P()
{
}

void OsmAnd::BinaryMapRasterLayerProvider_P::initialize()
{
    _mapRasterizer.reset(new MapRasterizer(owner->primitivesProvider->primitiviser->environment));
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterLayerProvider_P::getMinZoom() const
{
    return owner->primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterLayerProvider_P::getMaxZoom() const
{
    return owner->primitivesProvider->getMaxZoom();
}

bool OsmAnd::BinaryMapRasterLayerProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<BinaryMapRasterLayerProvider::Data>& outTiledData,
    BinaryMapRasterLayerProvider_Metrics::Metric_obtainData* const metric_,
    const IQueryController* const queryController)
{
#if OSMAND_PERFORMANCE_METRICS
    BinaryMapRasterLayerProvider_Metrics::Metric_obtainData localMetric;
    const auto metric = metric_ ? metric_ : &localMetric;
#else
    const auto metric = metric_;
#endif

    // Obtain offline map primitives tile
    std::shared_ptr<BinaryMapPrimitivesProvider::Data> primitivesTile;
    owner->primitivesProvider->obtainData(tileId, zoom, primitivesTile, metric ? &metric->obtainBinaryMapPrimitivesMetric : nullptr, nullptr);
    if (!primitivesTile || primitivesTile->primitivisedArea->isEmpty())
    {
        outTiledData.reset();
        return true;
    }

    // Perform actual rasterization
    const auto bitmap = rasterize(tileId, zoom, primitivesTile, metric, queryController);
    if (!bitmap)
        return false;

    // Or supply newly rasterized tile
    outTiledData.reset(new BinaryMapRasterLayerProvider::Data(
        tileId,
        zoom,
        AlphaChannelData::NotPresent,
        owner->getTileDensityFactor(),
        bitmap,
        primitivesTile,
        new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata)));

    return true;
}

OsmAnd::BinaryMapRasterLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::BinaryMapRasterLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
