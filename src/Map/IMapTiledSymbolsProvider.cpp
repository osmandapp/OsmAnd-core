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
    std::shared_ptr<TiledMapSymbolsData> tiledMapSymbolsData;
    const auto result = obtainData(tileId, zoom, tiledMapSymbolsData, nullptr, queryController);
    outTiledData = tiledMapSymbolsData;

    return result;
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
