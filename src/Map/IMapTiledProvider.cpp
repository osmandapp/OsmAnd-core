#include "IMapTiledProvider.h"

OsmAnd::IMapTiledProvider::IMapTiledProvider( const MapTileDataType& dataType_ )
    : dataType(dataType_)
{
}

OsmAnd::IMapTiledProvider::~IMapTiledProvider()
{
}

OsmAnd::MapTile::MapTile( const MapTileDataType& dataType_, DataPtr data_, size_t rowLength_, uint32_t size_ )
    : _data(data_)
    , dataType(dataType_)
    , data(_data)
    , rowLength(rowLength_)
    , size(size_)
{
}

OsmAnd::MapTile::~MapTile()
{
}
