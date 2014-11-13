#include "BinaryMapObjectsMetricsLayerProvider.h"
#include "BinaryMapObjectsMetricsLayerProvider_P.h"

OsmAnd::BinaryMapObjectsMetricsLayerProvider::BinaryMapObjectsMetricsLayerProvider(
    const std::shared_ptr<BinaryMapObjectsProvider>& dataProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapObjectsMetricsLayerProvider_P(this))
    , dataProvider(dataProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapObjectsMetricsLayerProvider::~BinaryMapObjectsMetricsLayerProvider()
{
}

float OsmAnd::BinaryMapObjectsMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapObjectsMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapObjectsMetricsLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, queryController);
    outTiledData = tiledData;

    return result;
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObjectsMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapObjectsMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapObjectsMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const BinaryMapObjectsProvider::Data>& binaryMapData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelData_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , binaryMapData(binaryMapData_)
{
}

OsmAnd::BinaryMapObjectsMetricsLayerProvider::Data::~Data()
{
    release();
}
