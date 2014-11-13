#include "IMapObjectsProvider.h"

OsmAnd::IMapObjectsProvider::IMapObjectsProvider()
{
}

OsmAnd::IMapObjectsProvider::~IMapObjectsProvider()
{
}

bool OsmAnd::IMapObjectsProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<Data> tiledData;
    const auto result = obtainData(tileId, zoom, tiledData, pOutMetric, queryController);
    outTiledData = tiledData;
    return result;
}

OsmAnd::IMapObjectsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const MapFoundationType tileFoundation_,
    const QList< std::shared_ptr<const MapObject> >& mapObjects_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledDataProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , tileFoundation(tileFoundation_)
    , mapObjects(mapObjects_)
{
}

OsmAnd::IMapObjectsProvider::Data::~Data()
{
    release();
}
