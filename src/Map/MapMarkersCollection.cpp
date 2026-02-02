#include "MapMarkersCollection.h"
#include "MapMarkersCollection_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::MapMarkersCollection::MapMarkersCollection()
    : _p(new MapMarkersCollection_P(this))
    , priority(std::numeric_limits<int64_t>::min())
{
}

OsmAnd::MapMarkersCollection::~MapMarkersCollection()
{
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkersCollection::getMarkerById(int id) const
{
    return _p->getMarkerById(id);
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
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::MapMarkersCollection::getMinZoom() const
{
    return OsmAnd::MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::MapMarkersCollection::getMaxZoom() const
{
    return OsmAnd::MaxZoomLevel;
}

int64_t OsmAnd::MapMarkersCollection::getPriority() const
{
    return priority;
}

void OsmAnd::MapMarkersCollection::setPriority(int64_t priority_)
{
    priority = priority_;
}
