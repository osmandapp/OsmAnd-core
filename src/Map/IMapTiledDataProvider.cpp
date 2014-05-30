#include "IMapTiledDataProvider.h"

OsmAnd::IMapTiledDataProvider::IMapTiledDataProvider(const DataType dataType_)
    : IMapDataProvider(dataType_)
{
}

OsmAnd::IMapTiledDataProvider::~IMapTiledDataProvider()
{
}

OsmAnd::MapTiledData::MapTiledData(const DataType dataType_, const TileId tileId_, const ZoomLevel zoom_)
    : dataType(dataType_)
    , tileId(tileId_)
    , zoom(zoom_)
{
}

OsmAnd::MapTiledData::~MapTiledData()
{
}
