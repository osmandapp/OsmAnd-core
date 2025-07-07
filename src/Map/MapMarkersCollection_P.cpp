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

QList< std::shared_ptr<OsmAnd::MapMarker> > OsmAnd::MapMarkersCollection_P::getMarkers() const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markers.values();
}

bool OsmAnd::MapMarkersCollection_P::addMarker(const std::shared_ptr<MapMarker>& marker)
{
    QWriteLocker scopedLocker(&_markersLock);

    const auto key = reinterpret_cast<IMapKeyedSymbolsProvider::Key>(marker.get());
    if (_markers.contains(key))
        return false;

    _markers.insert(key, marker);

    return true;
}

bool OsmAnd::MapMarkersCollection_P::removeMarker(const std::shared_ptr<MapMarker>& marker)
{
    QWriteLocker scopedLocker(&_markersLock);

    const bool removed = (_markers.remove(reinterpret_cast<IMapKeyedSymbolsProvider::Key>(marker.get())) > 0);
    return removed;
}

void OsmAnd::MapMarkersCollection_P::removeAllMarkers()
{
    QWriteLocker scopedLocker(&_markersLock);

    _markers.clear();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::MapMarkersCollection_P::getProvidedDataKeys() const
{
    QReadLocker scopedLocker(&_markersLock);

    return _markers.keys();
}

bool OsmAnd::MapMarkersCollection_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData)
{
    const auto& request = MapDataProviderHelpers::castRequest<MapMarkersCollection::Request>(request_);

    QReadLocker scopedLocker(&_markersLock);

    const auto citMarker = _markers.constFind(request.key);
    if (citMarker == _markers.cend())
        return false;
    auto& marker = *citMarker;

    outData.reset(new IMapKeyedSymbolsProvider::Data(request.key, marker->createSymbolsGroup(owner->subsection)));

    return true;
}
