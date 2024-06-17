#include "NetworkRouteContext_P.h"
#include "NetworkRouteContext.h"

#include "Road.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"
#include "Utilities.h"

OsmAnd::NetworkRouteContext_P::NetworkRouteContext_P(NetworkRouteContext* const owner_)
    : owner(owner_)
{
}


OsmAnd::NetworkRouteContext_P::~NetworkRouteContext_P()
{
}

QHash<OsmAnd::NetworkRouteKey, QList<std::shared_ptr<OsmAnd::NetworkRouteSegment>>> OsmAnd::NetworkRouteContext_P::loadRouteSegmentsBbox(AreaI area31, NetworkRouteKey * rKey)
{
    QHash<NetworkRouteKey, QList<std::shared_ptr<NetworkRouteSegment>>> map;
    int32_t left = area31.left() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    int32_t right = area31.right() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    int32_t top = area31.top() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    int32_t bottom = area31.bottom() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    for (int32_t x = left; x <= right; x++)
    {
        for (int32_t y = top; y <= bottom; y++)
        {
            loadRouteSegmentIntersectingTile(x, y, rKey, map);
        }
    }
    return map;
}

void OsmAnd::NetworkRouteContext_P::loadRouteSegmentIntersectingTile(int32_t x, int32_t y, const NetworkRouteKey * routeKey,
                                                         QHash<NetworkRouteKey, QList<std::shared_ptr<OsmAnd::NetworkRouteSegment>>> & map)
{
    NetworkRoutesTile osmcRoutesTile = getMapRouteTile(x << ZOOM_TO_LOAD_TILES_SHIFT_L, y << ZOOM_TO_LOAD_TILES_SHIFT_L);
    for (auto u_it = osmcRoutesTile.uniqueSegments.begin(); u_it != osmcRoutesTile.uniqueSegments.end(); ++u_it)
    {
        const auto &segment = u_it.value();
        if (routeKey != nullptr && segment->routeKey != *routeKey)
        {
            continue;
        }
        auto i = map.find(segment->routeKey);
        if (i == map.end())
        {
            QList<std::shared_ptr<NetworkRouteSegment>> empty;
            i = map.insert(segment->routeKey, empty);
        }
        i.value().append(segment);
    }
}

OsmAnd::NetworkRouteContext_P::NetworkRoutesTile OsmAnd::NetworkRouteContext_P::getMapRouteTile(int32_t x31, int32_t y31)
{
    int64_t tileId = getTileId(x31, y31);
    auto iterator = indexedTiles.find(tileId);
    if (iterator == indexedTiles.end())
    {
        const auto t = loadTile(x31 >> ZOOM_TO_LOAD_TILES_SHIFT_R, y31 >> ZOOM_TO_LOAD_TILES_SHIFT_R, tileId);
        iterator = indexedTiles.insert(tileId, t);
    }
    return *iterator;
}

int64_t OsmAnd::NetworkRouteContext_P::getTileId(int32_t x31, int32_t y31)
{
    return getTileId(x31, y31, ZOOM_TO_LOAD_TILES_SHIFT_R);
}

int64_t OsmAnd::NetworkRouteContext_P::getTileId(int32_t x, int32_t y, int shiftR)
{
    return (((int64_t) x >> shiftR) << ZOOM_TO_LOAD_TILES_SHIFT_L) + (int64_t) (y >> shiftR);
}

int32_t OsmAnd::NetworkRouteContext_P::getXFromTileId(int64_t tileId)
{
    return (int32_t) (tileId >> ZOOM_TO_LOAD_TILES_SHIFT_R);
}

int32_t OsmAnd::NetworkRouteContext_P::getYFromTileId(int64_t tileId)
{
    int64_t xShifted = tileId >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    return (int32_t) (tileId - (xShifted << ZOOM_TO_LOAD_TILES_SHIFT_L));
}

OsmAnd::NetworkRouteContext_P::NetworkRoutesTile OsmAnd::NetworkRouteContext_P::loadTile(int32_t x, int32_t y, int64_t tileId)
{
    //top, left, bottom, right
    AreaI area31(y << ZOOM_TO_LOAD_TILES_SHIFT_L, x << ZOOM_TO_LOAD_TILES_SHIFT_L, (y + 1) << ZOOM_TO_LOAD_TILES_SHIFT_L, (x + 1) << ZOOM_TO_LOAD_TILES_SHIFT_L);
    
    QList<std::shared_ptr<const Road>> roads;

    const auto obfDataInterface = owner->obfsCollection->obtainDataInterface(
        &area31,
        MinZoomLevel,
        MaxZoomLevel,
        ObfDataTypesMask().set(ObfDataType::Routing));
   
    obfDataInterface->loadRoads(
        RoutingDataLevel::Detailed,
        &area31,
        &roads,
        nullptr,
        nullptr,//visitor
        owner->cache.get(),
        nullptr,
        nullptr,
        nullptr);
    
    NetworkRoutesTile osmcRoutesTile(tileId);
    auto it = roads.begin();
    while (it != roads.end())
    {
        auto & road = *it;
        if (road == nullptr)
            continue;
        QVector<NetworkRouteKey> keys = convert(road);
        for (auto & rk : keys)
        {
            osmcRoutesTile.add(road, rk);
        }
        it++;
    }
    return osmcRoutesTile;
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext_P::convert(std::shared_ptr<const Road> & road) const
{
    return filterKeys(NetworkRouteKey::getRouteKeys(road));
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext_P::filterKeys(QVector<OsmAnd::NetworkRouteKey> keys) const
{
    if (keys.size() == 0 || (owner->filter.keyFilter.empty() && owner->filter.typeFilter.empty()))
    {
        return  keys;
    }
    
    QVector<OsmAnd::NetworkRouteKey> result;
    for (auto & key : keys)
    {
        if (owner->filter.keyFilter.contains(key))
        {
            result.append(key);
        }
        else if (owner->filter.typeFilter.contains(*key.type))
        {
            result.append(key);
        }
    }
    return result;
}

int64_t OsmAnd::NetworkRouteContext_P::convertPointToLong(int x31, int y31)
{
    return (((int64_t) x31) << 32) + y31;
}

void OsmAnd::NetworkRouteContext_P::NetworkRoutesTile::add(const std::shared_ptr<const Road> road, NetworkRouteKey &routeKey)
{
    int len = road->points31.size();
    bool inter = false;
    int px = 0, py = 0;
    for (int i = 0; i < len; i++)
    {
        int32_t x31 = road->points31[i].x;
        int32_t y31 = road->points31[i].y;
        inter = inter || (i > 0 && intersects(x31, y31, px, py));
        if (getTileId(x31, y31) != tileId)
        {
            px = x31;
            py = y31;
            continue;
        }
        inter = true;
        int64_t id = convertPointToLong(x31, y31);
        auto it = routes.find(id);
        if (it == routes.end())
        {
            auto point = std::make_shared<NetworkRoutePoint>(x31, y31, id);
            it = routes.insert(id, point);
        }
        auto & point = *it;
        if (i > 0)
        {
            addObjectToPoint(point, road, routeKey, i, 0);
        }
        if (i < len - 1)
        {
            addObjectToPoint(point, road, routeKey, i, len - 1);
        }
    }
    if (inter)
    {
        auto segment = std::make_shared<NetworkRouteSegment>(road, routeKey, 0, len - 1);
        addUnique(segment);
    }
}

bool OsmAnd::NetworkRouteContext_P::NetworkRoutesTile::intersects(int x31, int y31, int px, int py) const
{
    int64_t currentTile = getTileId(x31, y31);
    int64_t previousTile = getTileId(px, py);
    if (currentTile == tileId || previousTile == tileId)
    {
        return true;
    }
    if (currentTile == previousTile)
    {
        return false;
    }
    int32_t xprevTile = getXFromTileId(previousTile);
    int32_t yprevTile = getYFromTileId(previousTile);
    int32_t xcurrTile = getXFromTileId(currentTile);
    int32_t ycurrTile = getYFromTileId(currentTile);
    if ((ycurrTile == yprevTile && std::abs(xcurrTile - xprevTile) <= 1)
        || (xcurrTile == xprevTile && std::abs(ycurrTile - yprevTile) <= 1))
    {
        // speed up for neighbor tiles that couldn't intersect tileId
        return false;
    }
    
    if (std::abs(x31 - px) <= 2 && std::abs(y31 - py) <= 2)
    {
        // return when points too close to avoid rounding int errors
        return false;
    }
    // use recursive method to quickly find intersection
    if (intersects(x31, y31, x31 / 2 + px / 2, y31 / 2 + py / 2)) {
        return true;
    }
    if (intersects(px, py, x31 / 2 + px / 2, y31 / 2 + py / 2)) {
        return true;
    }
    return false;
}

void OsmAnd::NetworkRouteContext_P::NetworkRoutesTile::addUnique(const std::shared_ptr<NetworkRouteSegment> & networkRouteSegment)
{
    const QString key = networkRouteSegment->routeKey.toString() + QStringLiteral("_") + QString::number(networkRouteSegment->robj->id.id);
    uniqueSegments.insert(key, networkRouteSegment);
}

void OsmAnd::NetworkRouteContext_P::addObjectToPoint(const std::shared_ptr<NetworkRoutePoint> &point, const std::shared_ptr<const Road> road, NetworkRouteKey & routeKey, int start, int end)
{
    const auto seg = std::make_shared<NetworkRouteSegment>(road, routeKey, start, end);
    if (road->id > 0)
    {
        for (auto & seg2 : point->objects)
        {
            if (seg->robj->id == seg2->robj->id && seg->direction() == seg2->direction() && seg->routeKey == seg2->routeKey)
            {
                return;
            }
        }
    }
    point->objects.append(seg);
}

OsmAnd::PointI OsmAnd::NetworkRouteContext_P::getPointFromLong(int64_t l) const
{
    int32_t x = (int32_t) (l >> 32);
    int32_t y = (int32_t) (l - ((l >> 32) << 32));
    PointI point(x, y);
    return point;
}
