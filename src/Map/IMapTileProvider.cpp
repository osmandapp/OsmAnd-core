#include "IMapTileProvider.h"

OsmAnd::IMapTileProvider::IMapTileProvider( const Type& type_ )
    : type(type_)
{
}

OsmAnd::IMapTileProvider::~IMapTileProvider()
{
}

OsmAnd::IMapTileProvider::Tile::Tile( const Type& type_, const void* data_, size_t rowLength_, uint32_t width_, uint32_t height_ )
    : type(type_)
    , data(data_)
    , rowLength(rowLength_)
    , width(width_)
    , height(height_)
{
}

OsmAnd::IMapTileProvider::Tile::~Tile()
{
}
