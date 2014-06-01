#include "IMapKeyedDataProvider.h"

OsmAnd::IMapKeyedDataProvider::IMapKeyedDataProvider(const DataType dataType_)
    : IMapDataProvider(dataType_)
{
}

OsmAnd::IMapKeyedDataProvider::~IMapKeyedDataProvider()
{
}

OsmAnd::MapKeyedData::MapKeyedData(const DataType dataType_, const Key key_)
    : MapData(dataType_)
    , key(key_)
{
}

OsmAnd::MapKeyedData::~MapKeyedData()
{
}
