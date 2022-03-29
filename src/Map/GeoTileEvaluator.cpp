#include "GeoTileEvaluator.h"
#include "GeoTileEvaluator_P.h"

OsmAnd::GeoTileEvaluator::GeoTileEvaluator(
    const TileId geoTileId_,
    const QByteArray& geoTileData_,
    const ZoomLevel zoom_,
    const QString& projSearchPath_ /*= QString()*/)
    : _p(new GeoTileEvaluator_P(this))
    , geoTileId(geoTileId_)
    , geoTileData(geoTileData_)
    , zoom(zoom_)
    , projSearchPath(projSearchPath_)
{
}

OsmAnd::GeoTileEvaluator::~GeoTileEvaluator()
{
}

bool OsmAnd::GeoTileEvaluator::evaluate(
    const LatLon& latLon,
    QList<double>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/)
{
    return _p->evaluate(latLon, outData, pOutMetric, queryController);
}
