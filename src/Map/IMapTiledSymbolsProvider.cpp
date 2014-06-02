#include "IMapTiledSymbolsProvider.h"

OsmAnd::IMapTiledSymbolsProvider::IMapTiledSymbolsProvider()
    : IMapTiledDataProvider(DataType::SymbolsTile)
{
}

OsmAnd::IMapTiledSymbolsProvider::~IMapTiledSymbolsProvider()
{
}

bool OsmAnd::IMapTiledSymbolsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return obtainData(tileId, zoom, outTiledData, nullptr, queryController);
}

OsmAnd::TiledMapSymbolsData::TiledMapSymbolsData(const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_, const TileId tileId_, const ZoomLevel zoom_)
    : MapTiledData(DataType::SymbolsTile, tileId_, zoom_)
    , symbolsGroups(symbolsGroups_)
{
}

OsmAnd::TiledMapSymbolsData::~TiledMapSymbolsData()
{
}

void OsmAnd::TiledMapSymbolsData::releaseConsumableContent()
{
    symbolsGroups.clear();

    MapTiledData::releaseConsumableContent();
}
