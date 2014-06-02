#include "IMapDataProvider.h"

OsmAnd::IMapDataProvider::IMapDataProvider(const DataType dataType_)
    : dataType(dataType_)
{
}

OsmAnd::IMapDataProvider::~IMapDataProvider()
{
}

OsmAnd::MapData::MapData(const DataType dataType_)
    : _consumableContentReleased(false)
    , dataType(dataType_)
    , consumableContentReleased(_consumableContentReleased)
{
}

OsmAnd::MapData::~MapData()
{
}

void OsmAnd::MapData::releaseConsumableContent()
{
    _consumableContentReleased = true;
}
