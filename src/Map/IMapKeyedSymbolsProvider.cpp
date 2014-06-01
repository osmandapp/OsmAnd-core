#include "IMapKeyedSymbolsProvider.h"

OsmAnd::IMapKeyedSymbolsProvider::IMapKeyedSymbolsProvider()
    : IMapKeyedDataProvider(DataType::Symbols)
{
}

OsmAnd::IMapKeyedSymbolsProvider::~IMapKeyedSymbolsProvider()
{
}

OsmAnd::KeyedMapSymbolsData::KeyedMapSymbolsData(const std::shared_ptr<const MapSymbolsGroup>& group_, const Key key_)
    : MapKeyedData(DataType::Symbols, key_)
    , group(group_)
{
}

OsmAnd::KeyedMapSymbolsData::~KeyedMapSymbolsData()
{
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::KeyedMapSymbolsData::createNoContentInstance() const
{
    return nullptr;
}
