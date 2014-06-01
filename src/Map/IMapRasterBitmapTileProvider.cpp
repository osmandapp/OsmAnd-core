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
    assert((bitmap && bitmap->width() == bitmap->height()) || !bitmap);
}

OsmAnd::RasterBitmapTile::~RasterBitmapTile()
{
}

OsmAnd::RasterBitmapTile* OsmAnd::RasterBitmapTile::cloneWithBitmap(const std::shared_ptr<const SkBitmap>& newBitmap) const
{
    assert(bitmap->width() == newBitmap->width());
    assert(bitmap->height() == newBitmap->height());

    return new RasterBitmapTile(newBitmap, alphaChannelData, densityFactor, tileId, zoom);
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::RasterBitmapTile::createNoContentInstance() const
{
    // Metadata is useless in elevation data
    return nullptr;
}
