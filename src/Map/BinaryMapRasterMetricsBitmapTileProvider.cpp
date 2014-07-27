#include "BinaryMapRasterMetricsBitmapTileProvider.h"
#include "BinaryMapRasterMetricsBitmapTileProvider_P.h"

OsmAnd::BinaryMapRasterMetricsBitmapTileProvider::BinaryMapRasterMetricsBitmapTileProvider(
    const std::shared_ptr<BinaryMapRasterBitmapTileProvider>& rasterBitmapTileProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapRasterMetricsBitmapTileProvider_P(this))
    , rasterBitmapTileProvider(rasterBitmapTileProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapRasterMetricsBitmapTileProvider::~BinaryMapRasterMetricsBitmapTileProvider()
{
}

float OsmAnd::BinaryMapRasterMetricsBitmapTileProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapRasterMetricsBitmapTileProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapRasterMetricsBitmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsBitmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterMetricsBitmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapRasterMetricsBitmapTile::BinaryMapRasterMetricsBitmapTile(
    const std::shared_ptr<const BinaryMapRasterizedTile>& rasterizedMapTile_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const float densityFactor_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : RasterBitmapTile(bitmap_, AlphaChannelData::NotPresent, densityFactor_, tileId_, zoom_)
    , rasterizedMapTile(rasterizedMapTile_)
{
}

OsmAnd::BinaryMapRasterMetricsBitmapTile::~BinaryMapRasterMetricsBitmapTile()
{
}
