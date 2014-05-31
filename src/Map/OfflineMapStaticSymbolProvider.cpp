#include "OfflineMapStaticSymbolProvider.h"
#include "OfflineMapStaticSymbolProvider_P.h"

OsmAnd::OfflineMapStaticSymbolProvider::OfflineMapStaticSymbolProvider(const std::shared_ptr<BinaryMapDataProvider>& dataProvider_)
    : _p(new OfflineMapStaticSymbolProvider_P(this))
    , dataProvider(dataProvider_)
{
}

OsmAnd::OfflineMapStaticSymbolProvider::~OfflineMapStaticSymbolProvider()
{
}

bool OsmAnd::OfflineMapStaticSymbolProvider::obtainSymbols(
    const TileId tileId, const ZoomLevel zoom,
    std::shared_ptr<const MapTiledSymbols>& outTile,
    const FilterCallback filterCallback/* = nullptr*/)
{
    return _p->obtainSymbols(tileId, zoom, outTile, filterCallback);
}
