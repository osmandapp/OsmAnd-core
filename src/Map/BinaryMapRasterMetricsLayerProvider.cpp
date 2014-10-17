#include "BinaryMapRasterMetricsLayerProvider.h"
#include "BinaryMapRasterMetricsLayerProvider_P.h"

OsmAnd::BinaryMapRasterMetricsLayerProvider::BinaryMapRasterMetricsLayerProvider(
    const std::shared_ptr<BinaryMapRasterLayerProvider>& rasterBitmapTileProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapRasterMetricsLayerProvider_P(this))
    , rasterBitmapTileProvider(rasterBitmapTileProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapRasterMetricsLayerProvider::~BinaryMapRasterMetricsLayerProvider()
{
}

float OsmAnd::BinaryMapRasterMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapRasterMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapRasterMetricsLayerProvider::obtainData(
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

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapRasterMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const BinaryMapRasterLayerProvider::Data>& rasterizedBinaryMap_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelData_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , rasterizedBinaryMap(rasterizedBinaryMap_)
{
}

OsmAnd::BinaryMapRasterMetricsLayerProvider::Data::~Data()
{
    release();
}
