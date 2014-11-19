#include "MapPrimitivesMetricsLayerProvider.h"
#include "MapPrimitivesMetricsLayerProvider_P.h"

OsmAnd::MapPrimitivesMetricsLayerProvider::MapPrimitivesMetricsLayerProvider(
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new MapPrimitivesMetricsLayerProvider_P(this))
    , primitivesProvider(primitivesProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::MapPrimitivesMetricsLayerProvider::~MapPrimitivesMetricsLayerProvider()
{
}

float OsmAnd::MapPrimitivesMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::MapPrimitivesMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::MapPrimitivesMetricsLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    std::shared_ptr<MapPrimitivesMetricsLayerProvider::Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, queryController);
    outTiledData = tiledData;

    return result;
}

OsmAnd::ZoomLevel OsmAnd::MapPrimitivesMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapPrimitivesMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::MapPrimitivesMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelPresence alphaChannelPresence_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& binaryMapPrimitives_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelPresence_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , binaryMapPrimitives(binaryMapPrimitives_)
{
}

OsmAnd::MapPrimitivesMetricsLayerProvider::Data::~Data()
{
    release();
}
