#include "BinaryMapPrimitivesMetricsLayerProvider.h"
#include "BinaryMapPrimitivesMetricsLayerProvider_P.h"

OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::BinaryMapPrimitivesMetricsLayerProvider(
    const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapPrimitivesMetricsLayerProvider_P(this))
    , primitivesProvider(primitivesProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::~BinaryMapPrimitivesMetricsLayerProvider()
{
}

float OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<BinaryMapPrimitivesMetricsLayerProvider::Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, queryController);
    outTiledData = tiledData;
    return result;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& binaryMapPrimitives_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelData_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , binaryMapPrimitives(binaryMapPrimitives_)
{
}

OsmAnd::BinaryMapPrimitivesMetricsLayerProvider::Data::~Data()
{
    release();
}
