#include "NetworkRouteContext.h"
#include "NetworkRouteContext_P.h"

OsmAnd::NetworkRouteContext::NetworkRouteContext(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    NetworkRouteSelectorFilter filter_,
    const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache_)
    : _p(new NetworkRouteContext_P(this))
    , obfsCollection(obfsCollection_)
    , cache(cache_)
    , filter(filter_)
{
}

OsmAnd::NetworkRouteContext::~NetworkRouteContext()
{
}

QMap<OsmAnd::NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> OsmAnd::NetworkRouteContext::loadRouteSegmentsBbox(AreaI area31, NetworkRouteKey * rKey)
{
    return _p->loadRouteSegmentsBbox(area31, rKey);
}

OsmAnd::NetworkRouteKey::NetworkRouteKey(int index)
    : type(static_cast<RouteType>(index))
{
}

OsmAnd::NetworkRouteKey::~NetworkRouteKey()
{
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext::getRouteKeys(QHash<QString, QString> tags)
{
    return _p->getRouteKeys(tags);
}

int64_t OsmAnd::NetworkRouteContext::getTileId(int32_t x31, int32_t y31)
{
    return _p->getTileId(x31, y31);
}

int32_t OsmAnd::NetworkRouteContext::getXFromTileId(int64_t tileId)
{
    return 0;
}
int32_t OsmAnd::NetworkRouteContext::getYFromTileId(int64_t tileId)
{
    return 0;
}

void OsmAnd::NetworkRouteContext::loadRouteSegmentTile(int32_t x, int32_t y, NetworkRouteKey * routeKey,
                                                                        QMap<NetworkRouteKey, QList<NetworkRouteSegment>> & map)
{
    return _p->loadRouteSegmentTile(x, y, routeKey, map);
}

int64_t OsmAnd::NetworkRouteContext::getTileId(int32_t x31, int32_t y31, int shiftR)
{
    return _p->getTileId(x31, y31, shiftR);
}

int64_t OsmAnd::NetworkRouteContext::convertPointToLong(int x31, int y31)
{
    return _p->convertPointToLong(x31, y31);
}
