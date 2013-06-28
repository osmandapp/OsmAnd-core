#include "IMapElevationDataProvider.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider()
    : IMapTileProvider(IMapTileProvider::ElevationData)
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}
