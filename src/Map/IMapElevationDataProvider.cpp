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
        delete[] data;
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::ElevationDataTile::createNoContentInstance() const
{
    // Metadata is useless in elevation data
    return nullptr;
}
