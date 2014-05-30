#include "IMapTiledSymbolsProvider.h"

OsmAnd::IMapTiledSymbolsProvider::IMapTiledSymbolsProvider()
    : IMapTiledDataProvider(DataType::SymbolsTile)
{
}

OsmAnd::IMapTiledSymbolsProvider::~IMapTiledSymbolsProvider()
{
}

OsmAnd::MapTiledSymbols::MapTiledSymbols(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_, const TileId tileId_, const ZoomLevel zoom_)
    : MapTiledData(DataType::SymbolsTile, tileId_, zoom_)
    , symbolsGroups(symbolsGroups_)
{
}

OsmAnd::MapTiledSymbols::~MapTiledSymbols()
{
}
