#include "IMapBitmapTileProvider.h"

OsmAnd::IMapBitmapTileProvider::IMapBitmapTileProvider()
    : IMapTileProvider(IMapTileProvider::Bitmap)
{
}

OsmAnd::IMapBitmapTileProvider::~IMapBitmapTileProvider()
{
}

OsmAnd::IMapBitmapTileProvider::Tile::Tile( const void* data, size_t rowLength, uint32_t width, uint32_t height, const BitmapFormat& format_ )
    : IMapTileProvider::Tile(IMapTileProvider::Bitmap, data, rowLength, width, height)
    , format(format_)
{
}

OsmAnd::IMapBitmapTileProvider::Tile::~Tile()
{
}
