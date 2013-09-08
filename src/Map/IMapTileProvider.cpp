#include "IMapTileProvider.h"

OsmAnd::IMapTileProvider::IMapTileProvider( const MapTileDataType& dataType_ )
    : dataType(dataType_)
{
}

OsmAnd::IMapTileProvider::~IMapTileProvider()
{
}

OsmAnd::MapTile::MapTile( const MapTileDataType& dataType_, const void* data_, size_t rowLength_, uint32_t size_ )
    : dataType(dataType_)
    , data(data_)
    , rowLength(rowLength_)
    , size(size_)
{
}

OsmAnd::MapTile::~MapTile()
{
}
