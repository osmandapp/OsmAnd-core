#include "NetworkRouteContext.h"
#include "NetworkRouteContext_P.h"

OsmAnd::NetworkRouteContext::NetworkRouteContext(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache_)
    : _p(new NetworkRouteContext_P(this))
    , obfsCollection(obfsCollection_)
    , cache(cache_)
{
}

OsmAnd::NetworkRouteContext::~NetworkRouteContext()
{
}

void OsmAnd::NetworkRouteContext::setNetworkRouteKeyFilter(NetworkRouteKey & routeKey)
{
    filter.keyFilter.clear();
    filter.keyFilter.insert(routeKey);
}

OsmAnd::NetworkRouteSelectorFilter::NetworkRouteSelectorFilter()
{
}

OsmAnd::NetworkRouteSelectorFilter::NetworkRouteSelectorFilter(NetworkRouteSelectorFilter & other)
    :keyFilter(other.keyFilter), typeFilter(other.typeFilter)
{
}

OsmAnd::NetworkRouteSelectorFilter::~NetworkRouteSelectorFilter()
{    
}

QHash<OsmAnd::NetworkRouteKey, QList<std::shared_ptr<OsmAnd::NetworkRouteSegment>>> OsmAnd::NetworkRouteContext::loadRouteSegmentsBbox(AreaI area31, NetworkRouteKey * rKey)
{
    return _p->loadRouteSegmentsBbox(area31, rKey);
}

OsmAnd::NetworkRouteSegment::NetworkRouteSegment()
: start(0), end(0)
{
}

OsmAnd::NetworkRouteSegment::NetworkRouteSegment(NetworkRouteSegment & other)
: start(other.start), end(other.end), robj(other.robj), routeKey(other.routeKey)
{
}

OsmAnd::NetworkRouteSegment::NetworkRouteSegment(const NetworkRouteSegment & other)
: start(other.start), end(other.end), robj(other.robj), routeKey(other.routeKey)
{
}


OsmAnd::NetworkRouteSegment::NetworkRouteSegment(const std::shared_ptr<const Road> & road, const NetworkRouteKey & rKey, int start_, int end_)
: start(start_), end(end_), robj(road), routeKey(rKey)
{
}

OsmAnd::NetworkRouteSegment::~NetworkRouteSegment()
{
}

OsmAnd::NetworkRouteKey::NetworkRouteKey()
: type(static_cast<RouteType>(RouteType::Count))
{
}

OsmAnd::NetworkRouteKey::NetworkRouteKey(NetworkRouteKey & other)
: type(other.type), tags(other.tags)
{
}

OsmAnd::NetworkRouteKey::NetworkRouteKey(const NetworkRouteKey & other)
: type(other.type), tags(other.tags)
{
}

OsmAnd::NetworkRouteKey::NetworkRouteKey(int index)
: type(static_cast<RouteType>(index))
{
}

OsmAnd::NetworkRouteKey::~NetworkRouteKey()
{
}

OsmAnd::NetworkRoutePoint::~NetworkRoutePoint()
{    
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext::getRouteKeys(QHash<QString, QString> & tags) const
{
    return _p->getRouteKeys(tags);
}

int64_t OsmAnd::NetworkRouteContext::getTileId(int32_t x31, int32_t y31) const
{
    return _p->getTileId(x31, y31);
}

int32_t OsmAnd::NetworkRouteContext::getXFromTileId(int64_t tileId) const
{
    return _p->getXFromTileId(tileId);
}

int32_t OsmAnd::NetworkRouteContext::getYFromTileId(int64_t tileId) const
{
    return _p->getYFromTileId(tileId);
}

void OsmAnd::NetworkRouteContext::loadRouteSegmentIntersectingTile(int32_t x, int32_t y, const NetworkRouteKey * routeKey,
                                                                        QHash<NetworkRouteKey, QList<std::shared_ptr<NetworkRouteSegment>>> & map)
{
    return _p->loadRouteSegmentIntersectingTile(x, y, routeKey, map);
}

int64_t OsmAnd::NetworkRouteContext::getTileId(int32_t x31, int32_t y31, int shiftR) const
{
    return _p->getTileId(x31, y31, shiftR);
}

int64_t OsmAnd::NetworkRouteContext::convertPointToLong(int x31, int y31) const
{
    return _p->convertPointToLong(x31, y31);
}

int64_t OsmAnd::NetworkRouteContext::convertPointToLong(PointI point) const
{
    return _p->convertPointToLong(point.x, point.y);
}

OsmAnd::PointI OsmAnd::NetworkRouteContext::getPointFromLong(int64_t l) const
{
    return _p->getPointFromLong(l);
}

QMap<QString, QString> OsmAnd::NetworkRouteContext::tagsToGpx(const NetworkRouteKey & key) const
{
    return _p->tagsToGpx(key);
}

OsmAnd::Nullable<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext::fromGpx(const QMap<QString, QString> & networkRouteKeyTags) const
{
    return _p->fromGpx(networkRouteKeyTags);
}

QString OsmAnd::NetworkRouteKey::toString() const
{
    QString hash = QString::number(static_cast<int>(type));
    for (auto & s : tags)
    {
        hash += s;
    }
    return hash;
}
