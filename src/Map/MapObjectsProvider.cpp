#include "MapObjectsProvider.h"
#include "MapObjectsProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::MapObjectsProvider::MapObjectsProvider(const QList< std::shared_ptr<const MapObject> >& mapObjects_)
    : _p(new MapObjectsProvider_P(this))
    , mapObjects(mapObjects_)
{
    _p->prepareData();
}

OsmAnd::MapObjectsProvider::~MapObjectsProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapObjectsProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

bool OsmAnd::MapObjectsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::MapObjectsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::MapObjectsProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::MapObjectsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}
