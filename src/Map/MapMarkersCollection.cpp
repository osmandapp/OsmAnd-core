#include "MapMarkersCollection.h"
#include "MapMarkersCollection_P.h"

OsmAnd::MapMarkersCollection::MapMarkersCollection(const ZoomLevel minZoom_ /*= MinZoomLevel*/, const ZoomLevel maxZoom_ /*= MaxZoomLevel*/)
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

OsmAnd::ZoomLevel OsmAnd::MapMarkersCollection::getMinZoom() const
{
    return minZoom;
}

OsmAnd::ZoomLevel OsmAnd::MapMarkersCollection::getMaxZoom() const
{
    return maxZoom;
}

QList<OsmAnd::MapMarkersCollection::Key> OsmAnd::MapMarkersCollection::getProvidedDataKeys() const
{
    return _p->getProvidedDataKeys();
}

bool OsmAnd::MapMarkersCollection::obtainData(const Key key, std::shared_ptr<MapKeyedData>& outKeyedData, const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(key, outKeyedData, queryController);
}
