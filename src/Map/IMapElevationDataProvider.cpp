#include "IMapElevationDataProvider.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider()
    : IMapTileProvider(IMapTileProvider::ElevationData)
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}

OsmAnd::IMapElevationDataProvider::Tile::Tile( const float* data, size_t rowLength, uint32_t size )
    : IMapTileProvider::Tile(IMapTileProvider::ElevationData, data, rowLength, size, size)
{
}

OsmAnd::IMapElevationDataProvider::Tile::~Tile()
{
}
