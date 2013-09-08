#include "IMapElevationDataProvider.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider()
    : IMapTileProvider(MapTileDataType::ElevationData)
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}

OsmAnd::MapElevationDataTile::MapElevationDataTile( const float* data, size_t rowLength, uint32_t size )
    : MapTile(MapTileDataType::ElevationData, data, rowLength, size)
{
}

OsmAnd::MapElevationDataTile::~MapElevationDataTile()
{
    delete[] static_cast<const float*>(data);
}
