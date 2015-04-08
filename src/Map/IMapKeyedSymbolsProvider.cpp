#include "IMapKeyedSymbolsProvider.h"

#include "MapDataProviderHelpers.h"

OsmAnd::IMapKeyedSymbolsProvider::IMapKeyedSymbolsProvider()
{
}

OsmAnd::IMapKeyedSymbolsProvider::~IMapKeyedSymbolsProvider()
{
}

bool OsmAnd::IMapKeyedSymbolsProvider::obtainKeyedSymbols(
    const Request& request,
    std::shared_ptr<Data>& outKeyedSymbols,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outKeyedSymbols, pOutMetric);
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
