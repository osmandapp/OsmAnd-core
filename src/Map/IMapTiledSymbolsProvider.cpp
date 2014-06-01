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
    std::shared_ptr<const MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return obtainData(tileId, zoom, outTiledData, nullptr, queryController);
}

OsmAnd::TiledMapSymbolsData::TiledMapSymbolsData(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_, const TileId tileId_, const ZoomLevel zoom_)
    : MapTiledData(DataType::SymbolsTile, tileId_, zoom_)
    , symbolsGroups(symbolsGroups_)
{
}

OsmAnd::TiledMapSymbolsData::~TiledMapSymbolsData()
{
}

std::shared_ptr<OsmAnd::MapData> OsmAnd::TiledMapSymbolsData::createNoContentInstance() const
{
    return nullptr;
}
