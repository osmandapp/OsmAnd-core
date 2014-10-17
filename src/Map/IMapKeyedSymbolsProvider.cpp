#include "IMapKeyedSymbolsProvider.h"

OsmAnd::IMapKeyedSymbolsProvider::IMapKeyedSymbolsProvider()
{
}

OsmAnd::IMapKeyedSymbolsProvider::~IMapKeyedSymbolsProvider()
{
}

OsmAnd::IMapKeyedSymbolsProvider::Data::Data(
    const Key key_,
    const std::shared_ptr<MapSymbolsGroup>& symbolsGroup_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapKeyedDataProvider::Data(key_, pRetainableCacheMetadata_)
    , symbolsGroup(symbolsGroup_)
{
}

OsmAnd::IMapKeyedSymbolsProvider::Data::~Data()
{
    release();
}
