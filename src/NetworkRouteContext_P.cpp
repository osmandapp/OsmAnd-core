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

QMap<OsmAnd::NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> OsmAnd::NetworkRouteContext_P::loadRouteSegmentsBbox(AreaI area31, NetworkRouteKey * rKey)
{
    QMap<OsmAnd::NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> map;
    int32_t left = area31.left() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    int32_t right = area31.right() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    int32_t top = area31.top() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    int32_t bottom = area31.bottom() >> ZOOM_TO_LOAD_TILES_SHIFT_R;
    for (int32_t x = left; x <= right; x++)
    {
        for (int32_t y = top; y <= bottom; y++)
        {
            loadRouteSegmentTile(x, y, rKey, map);
        }
    }
    return map;
}

void OsmAnd::NetworkRouteContext_P::loadRouteSegmentTile(int32_t x, int32_t y, NetworkRouteKey * routeKey,
                                                         QMap<NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> & map)
{
    NetworkRoutesTile osmcRoutesTile = getMapRouteTile(x << ZOOM_TO_LOAD_TILES_SHIFT_L, y << ZOOM_TO_LOAD_TILES_SHIFT_L);
    for (auto & pnt : osmcRoutesTile.routes)
    {
        for (const auto & segment : pnt.objects)
        {
            if (routeKey != nullptr && segment.routeKey != *routeKey)
            {
                continue;
            }
          
            auto i = map.find(segment.routeKey);
            QList<OsmAnd::NetworkRouteSegment> routeSegments;
            if (i != map.end())
            {
                routeSegments = i.value();
            }

            if (segment.start == 0 || routeKey == nullptr) {
                routeSegments.append(segment);
                map.insert(segment.routeKey, routeSegments);
            }
        }
    }
    for (auto i = map.begin(); i != map.end(); ++i)
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Info, "key %s size %d", qPrintable((QString)i.key()), i.value().size());
    }
    return map;
}

OsmAnd::NetworkRouteContext_P::NetworkRoutesTile OsmAnd::NetworkRouteContext_P::getMapRouteTile(int32_t x31, int32_t y31)
{
    int64_t tileId = getTileId(x31, y31);
    auto iterator = indexedTiles.find(tileId);
    if (iterator == indexedTiles.end())
    {
        auto t = loadTile(x31 >> ZOOM_TO_LOAD_TILES_SHIFT_R, y31 >> ZOOM_TO_LOAD_TILES_SHIFT_R, tileId);
        indexedTiles.insert(tileId, t);
    }
    NetworkRoutesTile & tile = indexedTiles.find(tileId).value();
    return tile;
}

int64_t OsmAnd::NetworkRouteContext_P::getTileId(int32_t x31, int32_t y31) const
{
    return getTileId(x31, y31, ZOOM_TO_LOAD_TILES_SHIFT_R);
}

int64_t OsmAnd::NetworkRouteContext_P::getTileId(int32_t x, int32_t y, int shiftR) const
{
    return (((int64_t) x >> shiftR) << ZOOM_TO_LOAD_TILES_SHIFT_L) + (int64_t) (y >> shiftR);
}

int32_t OsmAnd::NetworkRouteContext_P::getXFromTileId(int64_t tileId) const
{
    return (int32_t) (tileId >> ZOOM_TO_LOAD_TILES_SHIFT_R);
}

int32_t OsmAnd::NetworkRouteContext_P::getYFromTileId(int64_t tileId) const
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
    /*ObfRoutingSectionReader::VisitorFunction visitor = [this, routeKey](const std::shared_ptr<const OsmAnd::Road>& road)
    {
        if (!routeKey)
        {
            return true;
        }
        QHash<QString, QString> tags = road->getResolvedAttributes();
        const auto & routeKeys = getRouteKeys(tags);
        if (routeKeys.contains(*routeKey))
        {
            return true;
        }
        return false;
    };*/
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
            addToTile(osmcRoutesTile, road, rk);
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
    if (owner->filter.keyFilter.empty() && owner->filter.typeFilter.empty())
    {
        return  keys;
    }
    
    QVector<OsmAnd::NetworkRouteKey> result;
    for (auto & key : keys)
    {
        if (!owner->filter.keyFilter.empty() && owner->filter.keyFilter.contains(key))
        {
            result.append(key);
        }
        else if (!owner->filter.typeFilter.empty() && owner->filter.typeFilter.contains(key.type))
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
            const QString prefix = tagPrefix + QString::number(routeIdx);
            NetworkRouteKey routeKey(ROUTE_TYPES_PREFIX.indexOf(tagPrefix));
            QHash<QString, QString>::iterator e;
            for (e = tags.begin(); e != tags.end(); ++e)
            {
                const QString tag = e.key();
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
        const auto& decodedAttribute = road->attributeMapping->decodeMap[attributeId];
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
    value = value.isEmpty() ? "" : ROUTE_KEY_VALUE_SEPARATOR + value;
    const QString tag = "route_" + getTag(routeKey) + ROUTE_KEY_VALUE_SEPARATOR + key + value;
    routeKey.tags.insert(tag);
}

QString OsmAnd::NetworkRouteContext_P::getTag(NetworkRouteKey & routeKey) const
{
    return ROUTE_TYPES_TAGS[static_cast<int>(routeKey.type)];
}

int64_t OsmAnd::NetworkRouteContext_P::convertPointToLong(int x31, int y31) const
{
    return (((int64_t) x31) << 32) + y31;
}

void OsmAnd::NetworkRouteContext_P::addToTile(NetworkRoutesTile & tile, std::shared_ptr<const Road> road, NetworkRouteKey & routeKey)
{
    int len = road->points31.size();
    for (int i = 0; i < len; i++)
    {
        int32_t x31 = road->points31[i].x;
        int32_t y31 = road->points31[i].y;
        if (getTileId(x31, y31) != tile.tileId)
        {
            continue;
        }
        int64_t id = convertPointToLong(x31, y31);
        auto it = tile.routes.find(id);
        if (it == tile.routes.end())
        {
            NetworkRoutePoint point(x31, y31, id);
            tile.routes.insert(id, point);
        }
        NetworkRoutePoint & point = *(tile.routes.find(id));
        if (i > 0)
        {
            addObjectToPoint(point, road, routeKey, i, 0);
        }
        if (i < len - 1)
        {
            addObjectToPoint(point, road, routeKey, i, len - 1);
        }
    }
}

void OsmAnd::NetworkRouteContext_P::addObjectToPoint(NetworkRoutePoint & point, std::shared_ptr<const Road> road, NetworkRouteKey & routeKey, int start, int end) const
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

OsmAnd::NetworkRouteKey * OsmAnd::NetworkRouteContext_P::fromGpx(const QMap<QString, QString> & networkRouteKeyTags) const
{
    auto it = networkRouteKeyTags.find(NETWORK_ROUTE_TYPE);
    if (it != networkRouteKeyTags.end())
    {
        QString type = *it;
        int rtype = ROUTE_TYPES_TAGS.indexOf(type);
        if (rtype < static_cast<int>(RouteType::Count))
        {
            NetworkRouteKey * routeKey = new NetworkRouteKey(rtype);
            for (auto i = networkRouteKeyTags.begin(); i != networkRouteKeyTags.end(); ++i)
            {
                addTag(*routeKey, i.key(), i.value());
            }
            return routeKey;
        }
    }
    return nullptr;
}

QString OsmAnd::NetworkRouteContext_P::getKeyFromTag(const NetworkRouteKey & key, QString tag) const
{
    QString prefix = "route_" + ROUTE_TYPES_TAGS[static_cast<int>(key.type)] + ROUTE_KEY_VALUE_SEPARATOR;
    if (tag.startsWith(prefix) && tag.length() > prefix.length())
    {
        int endIdx = tag.indexOf(ROUTE_KEY_VALUE_SEPARATOR, prefix.length());
        return tag.mid(prefix.length(), prefix.length() + endIdx);
    }
    return "";
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
    return "";
}
