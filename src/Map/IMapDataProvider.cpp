#include "IMapDataProvider.h"

OsmAnd::IMapDataProvider::IMapDataProvider(const DataType dataType_)
    : dataType(dataType_)
{
}

OsmAnd::IMapDataProvider::~IMapDataProvider()
{
}

OsmAnd::MapData::MapData(const DataType dataType_)
    : dataType(dataType_)
{
}

OsmAnd::MapData::~MapData()
{
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::MapData::createNoContentInstance() const
{
    return nullptr;
}
