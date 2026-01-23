#include "IMapTiledSymbolsProvider.h"

#include "MapDataProviderHelpers.h"

OsmAnd::IMapTiledSymbolsProvider::IMapTiledSymbolsProvider()
{
}

OsmAnd::IMapTiledSymbolsProvider::~IMapTiledSymbolsProvider()
{
}

bool OsmAnd::IMapTiledSymbolsProvider::obtainTiledSymbols(
    const Request& request,
    std::shared_ptr<Data>& outTiledSymbols,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outTiledSymbols, pOutMetric);
}

OsmAnd::IMapTiledSymbolsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledDataProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , symbolsGroups(symbolsGroups_)
{
}

OsmAnd::IMapTiledSymbolsProvider::Data::~Data()
{
    release();
}

OsmAnd::IMapTiledSymbolsProvider::Request::Request()
{
}

OsmAnd::IMapTiledSymbolsProvider::Request::Request(const IMapDataProvider::Request& that)
    : IMapTiledDataProvider::Request(that)
{
    copy(*this, that);
}

OsmAnd::IMapTiledSymbolsProvider::Request::Request(const Request& that)
    : IMapTiledDataProvider::Request(that)
{
    copy(*this, that);
}

OsmAnd::IMapTiledSymbolsProvider::Request::~Request()
{
}

void OsmAnd::IMapTiledSymbolsProvider::Request::copy(Request& dst, const IMapDataProvider::Request& src_)
{
    const auto& src = MapDataProviderHelpers::castRequest<Request>(src_);

    dst.filterCallback = src.filterCallback;
    dst.combineTilesData = src.combineTilesData;
}

std::shared_ptr<OsmAnd::IMapDataProvider::Request> OsmAnd::IMapTiledSymbolsProvider::Request::clone() const
{
    return std::shared_ptr<IMapDataProvider::Request>(new Request(*this));
}

bool OsmAnd::IMapTiledSymbolsProvider::waitForLoading() const
{
    return true;
}
