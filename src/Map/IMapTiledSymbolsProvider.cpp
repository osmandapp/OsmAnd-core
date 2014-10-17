#include "IMapTiledSymbolsProvider.h"

OsmAnd::IMapTiledSymbolsProvider::IMapTiledSymbolsProvider()
{
}

OsmAnd::IMapTiledSymbolsProvider::~IMapTiledSymbolsProvider()
{
}

bool OsmAnd::IMapTiledSymbolsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<Data> tiledData;
    const auto result = obtainData(tileId, zoom, tiledData, nullptr, queryController);
    outTiledData = tiledData;
    return result;
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
