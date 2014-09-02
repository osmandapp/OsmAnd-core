#include "BinaryMapStaticSymbolsProvider.h"
#include "BinaryMapStaticSymbolsProvider_P.h"

#include "BinaryMapPrimitivesProvider.h"

OsmAnd::BinaryMapStaticSymbolsProvider::BinaryMapStaticSymbolsProvider(
    const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_,
    const unsigned int referenceTileSizeInPixels_)
    : _p(new BinaryMapStaticSymbolsProvider_P(this))
    , primitivesProvider(primitivesProvider_)
    , referenceTileSizeInPixels(referenceTileSizeInPixels_)
{
}

OsmAnd::BinaryMapStaticSymbolsProvider::~BinaryMapStaticSymbolsProvider()
{
}

bool OsmAnd::BinaryMapStaticSymbolsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<TiledMapSymbolsData>& outTiledData,
    const FilterCallback filterCallback /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, filterCallback, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapStaticSymbolsProvider::getMinZoom() const
{
    return primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapStaticSymbolsProvider::getMaxZoom() const
{
    return primitivesProvider->getMaxZoom();
}

OsmAnd::BinaryMapStaticSymbolsTile::BinaryMapStaticSymbolsTile(
    const std::shared_ptr<const BinaryMapPrimitivesTile>& dataTile_,
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
