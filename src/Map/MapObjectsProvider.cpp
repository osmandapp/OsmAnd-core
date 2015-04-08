#include "MapObjectsProvider.h"
#include "MapObjectsProvider_P.h"

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

bool OsmAnd::MapObjectsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}
