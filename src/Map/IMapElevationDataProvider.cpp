#include "IMapElevationDataProvider.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider()
    : IMapTileProvider(IMapTileProvider::ElevationData)
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}

OsmAnd::IMapElevationDataProvider::Tile::Tile( const float* data, size_t rowLength, uint32_t width, uint32_t height )
    : IMapTileProvider::Tile(IMapTileProvider::ElevationData, data, rowLength, width, height)
{
}

OsmAnd::IMapElevationDataProvider::Tile::~Tile()
{
}
