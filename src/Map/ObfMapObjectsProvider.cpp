#include "ObfMapObjectsProvider.h"
#include "ObfMapObjectsProvider_P.h"

#include "ObfMapObjectsProvider_Metrics.h"
#include "MapDataProviderHelpers.h"

OsmAnd::ObfMapObjectsProvider::ObfMapObjectsProvider(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const Mode mode_ /*= Mode::BinaryMapObjectsAndRoads*/,
    const int threadsLimit_ /*= 0*/)
    : _p(new ObfMapObjectsProvider_P(this))
    , obfsCollection(obfsCollection_)
    , mode(mode_)
    , threadsLimit(threadsLimit_)
{
}

OsmAnd::ObfMapObjectsProvider::~ObfMapObjectsProvider()
{
}

bool OsmAnd::ObfMapObjectsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::ObfMapObjectsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::ObfMapObjectsProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::ObfMapObjectsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

bool OsmAnd::ObfMapObjectsProvider::obtainTiledObfMapObjects(
    const Request& request,
    std::shared_ptr<Data>& outMapObjects,
    ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric /*= nullptr*/)
{
    return _p->obtainTiledObfMapObjects(request, outMapObjects, metric);
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsProvider::getMinZoom() const
{
    return MinZoomLevel;//TODO: invalid
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsProvider::getMaxZoom() const
{
    return MaxZoomLevel;//TODO: invalid
}
