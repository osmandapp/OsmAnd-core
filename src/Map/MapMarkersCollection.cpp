#include "MapMarkersCollection.h"
#include "MapMarkersCollection_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::MapMarkersCollection::MapMarkersCollection(
    const ZoomLevel minZoom_ /*= MinZoomLevel*/,
    const ZoomLevel maxZoom_ /*= MaxZoomLevel*/)
    : _p(new MapMarkersCollection_P(this))
    , minZoom(minZoom_)
    , maxZoom(maxZoom_)
{
}

OsmAnd::MapMarkersCollection::~MapMarkersCollection()
{
}

QList< std::shared_ptr<OsmAnd::MapMarker> > OsmAnd::MapMarkersCollection::getMarkers() const
{
    return _p->getMarkers();
}

bool OsmAnd::MapMarkersCollection::removeMarker(const std::shared_ptr<MapMarker>& marker)
{
    return _p->removeMarker(marker);
}

void OsmAnd::MapMarkersCollection::removeAllMarkers()
{
    _p->removeAllMarkers();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::MapMarkersCollection::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
}

bool OsmAnd::MapMarkersCollection::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::MapMarkersCollection::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    if (pOutMetric)
        pOutMetric->reset();

    return _p->obtainData(request, outData);
}

bool OsmAnd::MapMarkersCollection::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::MapMarkersCollection::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}
