#include "IMapKeyedSymbolsProvider.h"

OsmAnd::IMapKeyedSymbolsProvider::IMapKeyedSymbolsProvider()
    : IMapKeyedDataProvider(DataType::Symbols)
{
}

OsmAnd::IMapKeyedSymbolsProvider::~IMapKeyedSymbolsProvider()
{
}

OsmAnd::KeyedSymbolsData::KeyedSymbolsData(const std::shared_ptr<MapSymbolsGroup>& group_, const Key key_)
    : MapKeyedData(DataType::Symbols, key_)
    , group(group_)
{
}

OsmAnd::KeyedSymbolsData::~KeyedSymbolsData()
{
}
