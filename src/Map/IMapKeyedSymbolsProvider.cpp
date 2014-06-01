#include "IMapKeyedSymbolsProvider.h"

OsmAnd::IMapKeyedSymbolsProvider::IMapKeyedSymbolsProvider()
    : IMapKeyedDataProvider(DataType::Symbols)
{
}

OsmAnd::IMapKeyedSymbolsProvider::~IMapKeyedSymbolsProvider()
{
}

OsmAnd::KeyedSymbolsData::KeyedSymbolsData(const std::shared_ptr<const MapSymbolsGroup>& group_, const Key key_)
    : MapKeyedData(DataType::Symbols, key_)
    , group(group_)
{
}

OsmAnd::KeyedSymbolsData::~KeyedSymbolsData()
{
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::KeyedSymbolsData::createNoContentInstance() const
{
    return nullptr;
}
