#include "BinaryMapMetricsBitmapTileProvider.h"
#include "BinaryMapMetricsBitmapTileProvider_P.h"

OsmAnd::BinaryMapMetricsBitmapTileProvider::BinaryMapMetricsBitmapTileProvider(
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapMetricsBitmapTileProvider_P(this))
    , dataProvider(dataProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapMetricsBitmapTileProvider::~BinaryMapMetricsBitmapTileProvider()
{
}

float OsmAnd::BinaryMapMetricsBitmapTileProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapMetricsBitmapTileProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapMetricsBitmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapMetricsBitmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapMetricsBitmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapMetricsTile::BinaryMapMetricsTile(
    const std::shared_ptr<const BinaryMapDataTile>& binaryMapTile_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const float densityFactor_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : RasterBitmapTile(bitmap_, AlphaChannelData::NotPresent, densityFactor_, tileId_, zoom_)
    , binaryMapTile(binaryMapTile_)
{
}

OsmAnd::BinaryMapMetricsTile::~BinaryMapMetricsTile()
{
}
