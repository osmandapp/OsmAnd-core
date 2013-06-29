#include "OneDegreeMapElevationDataProvider.h"

OsmAnd::OneDegreeMapElevationDataProvider::OneDegreeMapElevationDataProvider(const uint32_t& valuesPerSide)
    : IMapElevationDataProvider(valuesPerSide)
{
}

OsmAnd::OneDegreeMapElevationDataProvider::~OneDegreeMapElevationDataProvider()
{
}

bool OsmAnd::OneDegreeMapElevationDataProvider::obtainTileImmediate( const TileId& tileId, uint32_t zoom, std::shared_ptr<IMapTileProvider::Tile>& tile )
{
    // Elevation data tiles are not available immediately, since none of them are stored in memory.
    // In that case, a callback will be called
    return false;
}

void OsmAnd::OneDegreeMapElevationDataProvider::obtainTileDeffered( const TileId& tileId, uint32_t zoom, TileReadyCallback readyCallback )
{
}

