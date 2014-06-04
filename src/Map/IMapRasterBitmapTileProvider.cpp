#include "IMapRasterBitmapTileProvider.h"

#include <cassert>

#include <SkBitmap.h>

OsmAnd::IMapRasterBitmapTileProvider::IMapRasterBitmapTileProvider()
    : IMapTiledDataProvider(DataType::RasterBitmapTile)
{
}

OsmAnd::IMapRasterBitmapTileProvider::~IMapRasterBitmapTileProvider()
{
}

OsmAnd::RasterBitmapTile::RasterBitmapTile(
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapTiledData(DataType::RasterBitmapTile, tileId_, zoom_)
    , bitmap(bitmap_)
    , alphaChannelData(alphaChannelData_)
    , densityFactor(densityFactor)
{
    assert(bitmap->width() == bitmap->height());
}

OsmAnd::RasterBitmapTile::~RasterBitmapTile()
{
}

void OsmAnd::RasterBitmapTile::releaseConsumableContent()
{
    bitmap.reset();
    alphaChannelData = AlphaChannelData::Undefined;
    densityFactor = 0.0f;

    MapTiledData::releaseConsumableContent();
}
