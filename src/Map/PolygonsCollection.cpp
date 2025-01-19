#include "PolygonsCollection.h"
#include "PolygonsCollection_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::PolygonsCollection::PolygonsCollection(
	const ZoomLevel minZoom_,
	const ZoomLevel maxZoom_)
    : _p(new PolygonsCollection_P(this))
	, _minZoom(minZoom_)
	, _maxZoom(maxZoom_)
{
}

OsmAnd::PolygonsCollection::~PolygonsCollection()
{
}

QList< std::shared_ptr<OsmAnd::Polygon> > OsmAnd::PolygonsCollection::getPolygons() const
{
    return _p->getPolygons();
}

bool OsmAnd::PolygonsCollection::removePolygon(const std::shared_ptr<Polygon>& polygon)
{
    return _p->removePolygon(polygon);
}

void OsmAnd::PolygonsCollection::removeAllPolygons()
{
    _p->removeAllPolygons();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::PolygonsCollection::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
}

bool OsmAnd::PolygonsCollection::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::PolygonsCollection::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();
    
    return _p->obtainData(request, outData);
}

bool OsmAnd::PolygonsCollection::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::PolygonsCollection::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::PolygonsCollection::getMinZoom() const
{
    return _minZoom;
}

OsmAnd::ZoomLevel OsmAnd::PolygonsCollection::getMaxZoom() const
{
    return _maxZoom;
}
