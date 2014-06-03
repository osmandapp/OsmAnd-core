#include "IMapKeyedSymbolsProvider.h"

OsmAnd::IMapKeyedSymbolsProvider::IMapKeyedSymbolsProvider()
    : IMapKeyedDataProvider(DataType::Symbols)
{
}

OsmAnd::IMapKeyedSymbolsProvider::~IMapKeyedSymbolsProvider()
{
}

OsmAnd::KeyedMapSymbolsData::KeyedMapSymbolsData(const std::shared_ptr<MapSymbolsGroup>& symbolsGroup_, const Key key_)
    : MapKeyedData(DataType::Symbols, key_)
    , symbolsGroup(symbolsGroup_)
{
}

OsmAnd::KeyedMapSymbolsData::~KeyedMapSymbolsData()
{
}

void OsmAnd::KeyedMapSymbolsData::releaseConsumableContent()
{
    symbolsGroup.reset();

    MapKeyedData::releaseConsumableContent();
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::KeyedMapSymbolsData::clone() const
{
    return std::shared_ptr<MapData>(new KeyedMapSymbolsData(symbolsGroup, key));
}
