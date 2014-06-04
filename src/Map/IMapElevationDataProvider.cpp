#include "IMapElevationDataProvider.h"

OsmAnd::IMapElevationDataProvider::IMapElevationDataProvider()
    : IMapTiledDataProvider(DataType::ElevationDataTile)
{
}

OsmAnd::IMapElevationDataProvider::~IMapElevationDataProvider()
{
}

OsmAnd::ElevationDataTile::ElevationDataTile(
    const DataPtr data_,
    const size_t rowLength_,
    const uint32_t size_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapTiledData(DataType::ElevationDataTile, tileId_, zoom_)
    , data(data_)
    , rowLength(rowLength_)
    , size(size_)
{
}

OsmAnd::ElevationDataTile::~ElevationDataTile()
{
    if (data != nullptr)
    {
        delete[] data;
        data = nullptr;
    }
}

void OsmAnd::ElevationDataTile::releaseConsumableContent()
{
    if (data != nullptr)
    {
        delete[] data;
        data = nullptr;
    }
    rowLength = 0;
    size = 0;

    MapTiledData::releaseConsumableContent();
}
