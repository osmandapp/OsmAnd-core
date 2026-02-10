#include "PolygonsCollection_P.h"
#include "PolygonsCollection.h"

#include "MapDataProviderHelpers.h"
#include "Polygon.h"

OsmAnd::PolygonsCollection_P::PolygonsCollection_P(PolygonsCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::PolygonsCollection_P::~PolygonsCollection_P()
{
}

QList< std::shared_ptr<OsmAnd::Polygon> > OsmAnd::PolygonsCollection_P::getPolygons() const
{
    QReadLocker scopedLocker(&_polygonsLock);

    return _polygons.values();
}

bool OsmAnd::PolygonsCollection_P::addPolygon(const std::shared_ptr<Polygon>& polygon)
{
    QWriteLocker scopedLocker(&_polygonsLock);

    const auto key = reinterpret_cast<IMapKeyedSymbolsProvider::Key>(polygon->_p.get());
    if (_polygons.contains(key))
        return false;

    _polygons.insert(key, polygon);

    return true;
}

bool OsmAnd::PolygonsCollection_P::removePolygon(const std::shared_ptr<Polygon>& polygon)
{
    QWriteLocker scopedLocker(&_polygonsLock);

    const bool removed = (_polygons.remove(reinterpret_cast<IMapKeyedSymbolsProvider::Key>(polygon->_p.get())) > 0);
    return removed;
}

void OsmAnd::PolygonsCollection_P::removeAllPolygons()
{
    QWriteLocker scopedLocker(&_polygonsLock);

    _polygons.clear();
}

QList<OsmAnd::IMapKeyedSymbolsProvider::Key> OsmAnd::PolygonsCollection_P::getProvidedDataKeys() const
{
    QReadLocker scopedLocker(&_polygonsLock);

    return _polygons.keys();
}

bool OsmAnd::PolygonsCollection_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData)
{
    const auto& request = MapDataProviderHelpers::castRequest<PolygonsCollection::Request>(request_);

    QReadLocker scopedLocker(&_polygonsLock);

    const auto citPolygon = _polygons.constFind(request.key);
    if (citPolygon == _polygons.cend())
        return false;
    auto& polygon = *citPolygon;

    outData.reset(new IMapKeyedSymbolsProvider::Data(request.key, polygon->createSymbolsGroup(request.mapState)));

    return true;
}
