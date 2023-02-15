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

QHash<OsmAnd::NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> OsmAnd::NetworkRouteContext_P::loadRouteSegmentsBbox(AreaI area31, NetworkRouteKey * rKey)
{
    QHash<OsmAnd::NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> map;
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
                                                         QHash<NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> & map)
{
    NetworkRoutesTile osmcRoutesTile = getMapRouteTile(x << ZOOM_TO_LOAD_TILES_SHIFT_L, y << ZOOM_TO_LOAD_TILES_SHIFT_L);
    for (const auto & segment : osmcRoutesTile.uniqueSegments.values())
    {
        if (routeKey != nullptr && segment.routeKey != *routeKey)
        {
            continue;
        }
        auto i = map.find(segment.routeKey);
        if (i == map.end())
        {
            QList<OsmAnd::NetworkRouteSegment> empty;
            i = map.insert(segment.routeKey, empty);
        }
        auto & routeSegments = i.value();
        routeSegments.append(segment);
    }
}

OsmAnd::NetworkRouteContext_P::NetworkRoutesTile OsmAnd::NetworkRouteContext_P::getMapRouteTile(int32_t x31, int32_t y31)
{
    int64_t tileId = getTileId(x31, y31);
    auto iterator = indexedTiles.find(tileId);
    if (iterator == indexedTiles.end())
    {
        const auto & t = loadTile(x31 >> ZOOM_TO_LOAD_TILES_SHIFT_R, y31 >> ZOOM_TO_LOAD_TILES_SHIFT_R, tileId);
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
    return filterKeys(getRouteKeys(road));
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
        else if (owner->filter.typeFilter.contains(key.type))
        {
            result.append(key);
        }
    }
    return result;
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext_P::getRouteKeys(QHash<QString, QString> tags) const
{
    QVector<NetworkRouteKey> lst;
    for (const QString & tagPrefix : ROUTE_TYPES_PREFIX)
    {
        int rq = getRouteQuantity(tags, tagPrefix);
        for (int routeIdx = 1; routeIdx <= rq; routeIdx++)
        {
            const QString & prefix = tagPrefix + QString::number(routeIdx);
            NetworkRouteKey routeKey(ROUTE_TYPES_PREFIX.indexOf(tagPrefix));
            QHash<QString, QString>::iterator e;
            for (e = tags.begin(); e != tags.end(); ++e)
            {
                const QString & tag = e.key();
                if (tag.startsWith(prefix) && tag.length() > prefix.length())
                {
                    QString key = tag.right(tag.length() - (prefix.length() + 1));
                    addTag(routeKey, key, e.value());
                }
            }
            lst.append(routeKey);
        }
    }
    return lst;
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext_P::getRouteKeys(std::shared_ptr<const Road> & road) const
{
    QHash<QString, QString> tags = road->getResolvedAttributes();
    for (auto i = road->captions.begin(); i != road->captions.end(); ++i)
    {
        int attributeId = i.key();
        const auto & decodedAttribute = road->attributeMapping->decodeMap[attributeId];
        tags.insert(decodedAttribute.tag, i.value());
    }
    return getRouteKeys(tags);
}

int OsmAnd::NetworkRouteContext_P::getRouteQuantity(QHash<QString, QString> & tags, const QString tagPrefix) const
{
    int q = 0;
    QHash<QString, QString>::iterator e;
    for (e = tags.begin(); e != tags.end(); ++e)
    {
        QString tag = e.key();
        if (tag.startsWith(tagPrefix))
        {
            int num = OsmAnd::Utilities::extractIntegerNumber(tag);
            if (num > 0 && tag == (tagPrefix + QString::number(num)))
            {
                q = std::max(q, num);
            }
        }
        
    }
    return q;
}

void OsmAnd::NetworkRouteContext_P::addTag(NetworkRouteKey & routeKey, QString key, QString value) const
{
    value = value.isEmpty() ? QStringLiteral("") : ROUTE_KEY_VALUE_SEPARATOR + value;
    const QString tag = QStringLiteral("route_") + getTag(routeKey) + ROUTE_KEY_VALUE_SEPARATOR + key + value;
    routeKey.tags.insert(tag);
}

QString OsmAnd::NetworkRouteContext_P::getTag(NetworkRouteKey & routeKey) const
{
    return ROUTE_TYPES_TAGS[static_cast<int>(routeKey.type)];
}

int64_t OsmAnd::NetworkRouteContext_P::convertPointToLong(int x31, int y31)
{
    return (((int64_t) x31) << 32) + y31;
}

void OsmAnd::NetworkRouteContext_P::NetworkRoutesTile::add(std::shared_ptr<const Road> road, NetworkRouteKey & routeKey)
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
            continue;
        }
        inter = true;
        int64_t id = convertPointToLong(x31, y31);
        auto it = routes.find(id);
        if (it == routes.end())
        {
            NetworkRoutePoint point(x31, y31, id);
            it = routes.insert(id, point);
        }
        NetworkRoutePoint & point = *it;
        if (i > 0)
        {
            addObjectToPoint(point, road, routeKey, i, 0);
        }
        if (i < len - 1)
        {
            addObjectToPoint(point, road, routeKey, i, len - 1);
        }
        px = x31;
        py = y31;
    }
    if (inter)
    {
        NetworkRouteSegment segment(road, routeKey, 0, len - 1);
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

void OsmAnd::NetworkRouteContext_P::NetworkRoutesTile::addUnique(NetworkRouteSegment & networkRouteSegment)
{
    const QString & key = networkRouteSegment.routeKey.toString() + QStringLiteral("_") + QString::number(networkRouteSegment.robj->id.id);
    uniqueSegments.insert(key, networkRouteSegment);
}

void OsmAnd::NetworkRouteContext_P::addObjectToPoint(NetworkRoutePoint & point, std::shared_ptr<const Road> road, NetworkRouteKey & routeKey, int start, int end)
{
    OsmAnd::NetworkRouteSegment seg(road, routeKey, start, end);
    if (road->id > 0)
    {
        for (NetworkRouteSegment & seg2 : point.objects)
        {
            if (seg.robj->id == seg2.robj->id && seg.direction() == seg2.direction() && seg.routeKey == seg2.routeKey)
            {
                return;
            }
        }
    }
    point.objects.append(seg);
}

OsmAnd::PointI OsmAnd::NetworkRouteContext_P::getPointFromLong(int64_t l) const
{
    int32_t x = (int32_t) (l >> 32);
    int32_t y = (int32_t) (l - ((l >> 32) << 32));
    PointI point(x, y);
    return point;
}

QMap<QString, QString> OsmAnd::NetworkRouteContext_P::tagsToGpx(const NetworkRouteKey & routeKey) const
{
    QMap<QString, QString> networkRouteKey;
    networkRouteKey.insert(NETWORK_ROUTE_TYPE, ROUTE_TYPES_TAGS[static_cast<int>(routeKey.type)]);
    for (auto & tag : routeKey.tags)
    {
        QString key = getKeyFromTag(routeKey, tag);
        QString value = getValue(routeKey, key);
        if (!value.isEmpty())
        {
            networkRouteKey.insert(key, value);
        }
    }
    return networkRouteKey;
}

OsmAnd::Nullable<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext_P::fromGpx(const QMap<QString, QString> & networkRouteKeyTags) const
{
    auto it = networkRouteKeyTags.find(NETWORK_ROUTE_TYPE);
    if (it != networkRouteKeyTags.end())
    {
        QString type = *it;
        int rtype = ROUTE_TYPES_TAGS.indexOf(type);
        if (rtype < static_cast<int>(RouteType::Count))
        {
            NetworkRouteKey routeKey(rtype);
            for (auto i = networkRouteKeyTags.begin(); i != networkRouteKeyTags.end(); ++i)
            {
                addTag(routeKey, i.key(), i.value());
            }
            return Nullable<NetworkRouteKey>(routeKey);
        }
    }
    return Nullable<NetworkRouteKey>();
}

QString OsmAnd::NetworkRouteContext_P::getKeyFromTag(const NetworkRouteKey & key, QString tag) const
{
    QString prefix = QStringLiteral("route_") + ROUTE_TYPES_TAGS[static_cast<int>(key.type)] + ROUTE_KEY_VALUE_SEPARATOR;
    if (tag.startsWith(prefix) && tag.length() > prefix.length())
    {
        int endIdx = tag.indexOf(ROUTE_KEY_VALUE_SEPARATOR, prefix.length());
        return tag.mid(prefix.length(), prefix.length() + endIdx);
    }
    return QStringLiteral("");
}

QString OsmAnd::NetworkRouteContext_P::getValue(const NetworkRouteKey & routeKey, QString key) const
{
    key = ROUTE_KEY_VALUE_SEPARATOR + key + ROUTE_KEY_VALUE_SEPARATOR;
    for (auto & tag : routeKey.tags)
    {
        int i = tag.indexOf(key);
        if (i > 0)
        {
            return tag.right(tag.length() - (i + key.length()));
        }
    }
    return QStringLiteral("");
}
