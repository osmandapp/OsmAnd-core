#include "BinaryMapRasterBitmapTileProvider.h"
#include "BinaryMapRasterBitmapTileProvider_P.h"

OsmAnd::BinaryMapRasterBitmapTileProvider::BinaryMapRasterBitmapTileProvider(
    BinaryMapRasterBitmapTileProvider_P* const p_,
    const std::shared_ptr<BinaryMapDataProvider>& dataProvider_,
    const uint32_t tileSize_,
    const float densityFactor_)
    : _p(p_)
    , dataProvider(dataProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapRasterBitmapTileProvider::~BinaryMapRasterBitmapTileProvider()
{
}

float OsmAnd::BinaryMapRasterBitmapTileProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapRasterBitmapTileProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapRasterBitmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapRasterizedTile::BinaryMapRasterizedTile(
    const std::shared_ptr<const BinaryMapDataTile>& binaryMapTile_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : RasterBitmapTile(bitmap_, alphaChannelData_, densityFactor_, tileId_, zoom_)
    , binaryMapTile(binaryMapTile_)
{
}

OsmAnd::BinaryMapRasterizedTile::~BinaryMapRasterizedTile()
{
}
