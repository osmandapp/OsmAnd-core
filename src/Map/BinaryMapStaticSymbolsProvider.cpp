#include "BinaryMapStaticSymbolsProvider.h"
#include "BinaryMapStaticSymbolsProvider_P.h"

#include "BinaryMapDataProvider.h"

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
    std::shared_ptr<MapTiledData>& outTiledData,
    const FilterCallback filterCallback /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, filterCallback, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapStaticSymbolsProvider::getMinZoom() const
{
    return dataProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapStaticSymbolsProvider::getMaxZoom() const
{
    return dataProvider->getMaxZoom();
}

OsmAnd::BinaryMapStaticSymbolsTile::BinaryMapStaticSymbolsTile(
    const std::shared_ptr<const BinaryMapDataTile>& dataTile_,
    const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : TiledMapSymbolsData(symbolsGroups_, tileId_, zoom_)
    , dataTile(dataTile_)
{
}

OsmAnd::BinaryMapStaticSymbolsTile::~BinaryMapStaticSymbolsTile()
{
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::BinaryMapStaticSymbolsTile::clone() const
{
    return std::shared_ptr<MapData>(new BinaryMapStaticSymbolsTile(dataTile, symbolsGroups, tileId, zoom));
}
