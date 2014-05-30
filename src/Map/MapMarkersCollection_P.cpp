#include "MapMarkersCollection_P.h"
#include "MapMarkersCollection.h"

OsmAnd::MapMarkersCollection_P::MapMarkersCollection_P(MapMarkersCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapMarkersCollection_P::~MapMarkersCollection_P()
{
}

QList< std::shared_ptr<OsmAnd::MapMarker> > OsmAnd::MapMarkersCollection_P::getMarkers() const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markers.values();
}

bool OsmAnd::MapMarkersCollection_P::addMarker(const std::shared_ptr<MapMarker>& marker)
{
    QWriteLocker scopedLocker(&_markersLock);

    const auto key = reinterpret_cast<Key>(marker.get());
    if (_markers.contains(key))
        return false;

    _markers.insert(key, marker);

    return true;
}

bool OsmAnd::MapMarkersCollection_P::removeMarker(const std::shared_ptr<MapMarker>& marker)
{
    QWriteLocker scopedLocker(&_markersLock);

    const bool removed = (_markers.remove(reinterpret_cast<Key>(marker.get())) > 0);
    return removed;
}

void OsmAnd::MapMarkersCollection_P::removeAllMarkers()
{
    QWriteLocker scopedLocker(&_markersLock);

    _markers.clear();
}

QList<OsmAnd::MapMarkersCollection_P::Key> OsmAnd::MapMarkersCollection_P::getKeys() const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markers.keys();
}

bool OsmAnd::MapMarkersCollection_P::obtainSymbolsGroup(const Key key, std::shared_ptr<const MapSymbolsGroup>& outSymbolGroups)
{
    QReadLocker scopedLocker(&_markersLock);

    const auto citMarker = _markers.constFind(key);
    if (citMarker == _markers.cend())
        return false;

    //outSymbolGroups = ;

    return true;
}