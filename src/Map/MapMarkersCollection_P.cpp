#include "MapMarkersCollection_P.h"
#include "MapMarkersCollection.h"

#include "MapDataProviderHelpers.h"
#include "MapMarker.h"

OsmAnd::MapMarkersCollection_P::MapMarkersCollection_P(MapMarkersCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapMarkersCollection_P::~MapMarkersCollection_P()
{
}

std::shared_ptr<OsmAnd::MapMarker> OsmAnd::MapMarkersCollection_P::getMarkerById(int markerId, int groupId) const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markersById.value(groupId).value(markerId);
}

QList< std::shared_ptr<OsmAnd::MapMarker> > OsmAnd::MapMarkersCollection_P::getMarkers() const
{
    QReadLocker scopedLocker(&_markersLock);

    QList< std::shared_ptr<OsmAnd::MapMarker> > result;
    for (auto& markers : _markers)
    {
        result.append(markers.values());
    }

    return result;
}

int OsmAnd::MapMarkersCollection_P::getMarkersCountByGroupId(int groupId) const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markersById[groupId].size();
}

bool OsmAnd::MapMarkersCollection_P::addMarker(const std::shared_ptr<MapMarker>& marker)
{
    if (!marker)
        return false;

    QWriteLocker scopedLocker(&_markersLock);

    const auto key = reinterpret_cast<IMapKeyedSymbolsProvider::Key>(marker.get());
    if (_markers[marker->groupId].contains(key))
        return false;

    _markers[marker->groupId].insert(key, marker);
    _markersById[marker->groupId].insert(marker->markerId, marker);

    return true;
}

void OsmAnd::MapMarkersCollection_P::removeMarkersByGroupId(int groupId)
{
    QWriteLocker scopedLocker(&_markersLock);

    _markersById[groupId].clear();
    _markers[groupId].clear();
}

bool OsmAnd::MapMarkersCollection_P::removeMarkerById(int markerId, int groupId)
{
    QWriteLocker scopedLocker(&_markersLock);

    const auto marker = _markersById[groupId].value(markerId);
    if (!marker)
        return false;
    _markersById[groupId].remove(markerId);
    const bool removed =
        (_markers[groupId].remove(reinterpret_cast<IMapKeyedSymbolsProvider::Key>(marker.get())) > 0);
    return removed;
}

bool OsmAnd::MapMarkersCollection_P::removeMarker(const std::shared_ptr<MapMarker>& marker)
{
    if (!marker)
        return false;

    QWriteLocker scopedLocker(&_markersLock);

    _markersById[marker->groupId].remove(marker->markerId);
    const bool removed =
        (_markers[marker->groupId].remove(reinterpret_cast<IMapKeyedSymbolsProvider::Key>(marker.get())) > 0);
    return removed;
}

void OsmAnd::MapMarkersCollection_P::removeAllMarkers()
{
    QWriteLocker scopedLocker(&_markersLock);

    _markersById.clear();
    _markers.clear();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::MapMarkersCollection_P::getProvidedDataKeys() const
{
    QReadLocker scopedLocker(&_markersLock);

    QList<OsmAnd::IMapKeyedSymbolsProvider::Key> result;
    for (auto& markers : _markers)
    {
        result.append(markers.keys());
    }

    return result;
}

bool OsmAnd::MapMarkersCollection_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData)
{
    const auto& request = MapDataProviderHelpers::castRequest<MapMarkersCollection::Request>(request_);

    QReadLocker scopedLocker(&_markersLock);

    std::shared_ptr<MapMarker> marker;
    for (auto& markers : _markers)
    {
        const auto citMarker = markers.constFind(request.key);
        if (citMarker != markers.cend())
        {
            marker = *citMarker;
            break;
        }        
    }
    if (!marker)
        return false;

    outData.reset(new IMapKeyedSymbolsProvider::Data(request.key, marker->createSymbolsGroup(owner->subsection)));

    return true;
}
