#include "BinaryMapDataProvider.h"
#include "BinaryMapDataProvider_P.h"

#include "MapPresentationEnvironment.h"

OsmAnd::BinaryMapDataProvider::BinaryMapDataProvider(const std::shared_ptr<const IObfsCollection>& obfsCollection_)
    : _p(new BinaryMapDataProvider_P(this))
    , obfsCollection(obfsCollection_)
{
}

OsmAnd::BinaryMapDataProvider::~BinaryMapDataProvider()
{
}

bool OsmAnd::BinaryMapDataProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData_,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, nullptr, queryController);
    outTiledData_ = tiledData;
    return result;
}

bool OsmAnd::BinaryMapDataProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<Data>& outTiledData,
    BinaryMapDataProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataProvider::getMinZoom() const
{
    return MinZoomLevel;//TODO: invalid
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapDataProvider::getMaxZoom() const
{
    return MaxZoomLevel;//TODO: invalid
}

OsmAnd::BinaryMapDataProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const MapFoundationType tileFoundation_,
    const QList< std::shared_ptr<const Model::BinaryMapObject> >& mapObjects_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledDataProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , tileFoundation(tileFoundation_)
    , mapObjects(mapObjects_)
{
}

OsmAnd::BinaryMapDataProvider::Data::~Data()
{
    release();
}
