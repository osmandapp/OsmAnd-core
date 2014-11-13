#include "MapRasterMetricsLayerProvider.h"
#include "MapRasterMetricsLayerProvider_P.h"

OsmAnd::MapRasterMetricsLayerProvider::MapRasterMetricsLayerProvider(
    const std::shared_ptr<MapRasterLayerProvider>& rasterBitmapTileProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new MapRasterMetricsLayerProvider_P(this))
    , rasterBitmapTileProvider(rasterBitmapTileProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::MapRasterMetricsLayerProvider::~MapRasterMetricsLayerProvider()
{
}

float OsmAnd::MapRasterMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::MapRasterMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::MapRasterMetricsLayerProvider::obtainData(
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

OsmAnd::ZoomLevel OsmAnd::MapRasterMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapRasterMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::MapRasterMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const MapRasterLayerProvider::Data>& rasterizedBinaryMap_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelData_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , rasterizedBinaryMap(rasterizedBinaryMap_)
{
}

OsmAnd::MapRasterMetricsLayerProvider::Data::~Data()
{
    release();
}
