#include "MapMarkersCollection_P.h"
#include "MapMarkersCollection.h"

OsmAnd::MapMarkersCollection_P::MapMarkersCollection_P(MapMarkersCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapMarkersCollection_P::~MapMarkersCollection_P()
{
}

QSet< std::shared_ptr<OsmAnd::MapMarker> > OsmAnd::MapMarkersCollection_P::getMarkers() const
{
    return QSet< std::shared_ptr<OsmAnd::MapMarker> >();
}

bool OsmAnd::MapMarkersCollection_P::removeMarker(const std::shared_ptr<MapMarker>& marker)
{
    return false;
}

void OsmAnd::MapMarkersCollection_P::removeAllMarkers()
{

}

QSet<OsmAnd::MapMarkersCollection_P::Key> OsmAnd::MapMarkersCollection_P::getKeys() const
{
    return QSet<OsmAnd::MapMarkersCollection_P::Key>();
}

bool OsmAnd::MapMarkersCollection_P::obtainSymbolsGroup(const Key key, std::shared_ptr<const MapSymbolsGroup>& outSymbolGroups)
{
    return false;
}
