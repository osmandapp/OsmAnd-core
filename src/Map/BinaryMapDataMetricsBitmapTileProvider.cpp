#include "BinaryMapDataMetricsBitmapTileProvider.h"
#include "BinaryMapDataMetricsBitmapTileProvider_P.h"

OsmAnd::BinaryMapDataMetricsBitmapTileProvider::BinaryMapDataMetricsBitmapTileProvider(
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapDataMetricsBitmapTileProvider_P(this))
    , dataProvider(dataProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapDataMetricsBitmapTileProvider::~BinaryMapDataMetricsBitmapTileProvider()
{
}

float OsmAnd::BinaryMapDataMetricsBitmapTileProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapDataMetricsBitmapTileProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapDataMetricsBitmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataMetricsBitmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataMetricsBitmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapDataMetricsTile::BinaryMapDataMetricsTile(
    const std::shared_ptr<const BinaryMapDataTile>& binaryMapDataTile_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const float densityFactor_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : RasterBitmapTile(bitmap_, AlphaChannelData::NotPresent, densityFactor_, tileId_, zoom_)
    , binaryMapDataTile(binaryMapDataTile_)
{
}

OsmAnd::BinaryMapDataMetricsTile::~BinaryMapDataMetricsTile()
{
}
