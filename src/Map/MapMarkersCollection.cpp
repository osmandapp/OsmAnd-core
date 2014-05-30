#include "MapMarkersCollection.h"
#include "MapMarkersCollection_P.h"

OsmAnd::MapMarkersCollection::MapMarkersCollection()
    : _p(new MapMarkersCollection_P(this))
{
}

OsmAnd::MapMarkersCollection::~MapMarkersCollection()
{
}

QSet<OsmAnd::MapMarkersCollection::Key> OsmAnd::MapMarkersCollection::getKeys() const
{
    return _p->getKeys();
}

bool OsmAnd::MapMarkersCollection::obtainSymbolsGroup(const Key key, std::shared_ptr<const MapSymbolsGroup>& outSymbolGroups)
{
    return _p->obtainSymbolsGroup(key, outSymbolGroups);
}
