#include "IMapObjectsProvider.h"

#include "MapDataProviderHelpers.h"

OsmAnd::IMapObjectsProvider::IMapObjectsProvider()
{
}

OsmAnd::IMapObjectsProvider::~IMapObjectsProvider()
{
}

bool OsmAnd::IMapObjectsProvider::obtainTiledMapObjects(
    const Request& request,
    std::shared_ptr<Data>& outMapObjects,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outMapObjects, pOutMetric);
}

OsmAnd::IMapObjectsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const MapSurfaceType tileSurfaceType_,
    const QList< std::shared_ptr<const MapObject> >& mapObjects_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledDataProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , tileSurfaceType(tileSurfaceType_)
    , mapObjects(mapObjects_)
{
}

OsmAnd::IMapObjectsProvider::Data::~Data()
{
    release();
}
