#include "BinaryMapStaticSymbolsProvider.h"
#include "BinaryMapStaticSymbolsProvider_P.h"

OsmAnd::BinaryMapStaticSymbolsProvider::BinaryMapStaticSymbolsProvider(const std::shared_ptr<BinaryMapDataProvider>& dataProvider_)
    : _p(new BinaryMapStaticSymbolsProvider_P(this))
    , dataProvider(dataProvider_)
{
}

OsmAnd::BinaryMapStaticSymbolsProvider::~BinaryMapStaticSymbolsProvider()
{
}

bool OsmAnd::BinaryMapStaticSymbolsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<const MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

bool OsmAnd::BinaryMapStaticSymbolsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<const MapTiledData>& outTiledData,
    const FilterCallback filterCallback /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, filterCallback, queryController);
}

OsmAnd::BinaryMapStaticSymbolsTile::BinaryMapStaticSymbolsTile(
    const std::shared_ptr<const BinaryMapDataTile>& dataTile_,
    const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : MapTiledSymbols(symbolsGroups_, tileId_, zoom_)
    , dataTile(dataTile_)
{
}

OsmAnd::BinaryMapStaticSymbolsTile::~BinaryMapStaticSymbolsTile()
{
}
