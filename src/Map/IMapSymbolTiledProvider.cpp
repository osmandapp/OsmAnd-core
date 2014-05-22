#include "IMapSymbolTiledProvider.h"

OsmAnd::IMapSymbolTiledProvider::IMapSymbolTiledProvider()
{
}

OsmAnd::IMapSymbolTiledProvider::~IMapSymbolTiledProvider()
{
}

OsmAnd::MapSymbolsTile::MapSymbolsTile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_)
    : _symbolsGroups(symbolsGroups_)
    , symbolsGroups(_symbolsGroups)
{
}

OsmAnd::MapSymbolsTile::~MapSymbolsTile()
{
}
