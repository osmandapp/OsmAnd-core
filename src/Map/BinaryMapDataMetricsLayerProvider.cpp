#include "BinaryMapDataMetricsLayerProvider.h"
#include "BinaryMapDataMetricsLayerProvider_P.h"

OsmAnd::BinaryMapDataMetricsLayerProvider::BinaryMapDataMetricsLayerProvider(
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapDataMetricsLayerProvider_P(this))
    , dataProvider(dataProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapDataMetricsLayerProvider::~BinaryMapDataMetricsLayerProvider()
{
}

float OsmAnd::BinaryMapDataMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapDataMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapDataMetricsLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, queryController);
    outTiledData = tiledData;
    return result;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapDataMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const BinaryMapDataProvider::Data>& binaryMapData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelData_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , binaryMapData(binaryMapData_)
{
}

OsmAnd::BinaryMapDataMetricsLayerProvider::Data::~Data()
{
    release();
}
