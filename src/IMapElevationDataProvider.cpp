#include "IMapElevationDataProvider.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider(const uint32_t& valuesPerSide_)
    : IMapTileProvider(IMapTileProvider::ElevationData)
    , valuesPerSide(valuesPerSide_)
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}
