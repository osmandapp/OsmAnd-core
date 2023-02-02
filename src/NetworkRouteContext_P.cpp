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
                //TODO move filter to visitor function
                continue;
            }
          
            auto i = map.find(segment.routeKey);
            if (i == map.end())
            {
                QList<OsmAnd::NetworkRouteSegment> routeSegments;
                map.insert(segment.routeKey, routeSegments);
            }
            
            if (segment.start == 0 || routeKey == nullptr) {
                i = map.find(segment.routeKey);
                auto & routeSegments = i.value();
                routeSegments.append(segment);
            }
        }
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

int64_t OsmAnd::NetworkRouteContext_P::getTileId(int32_t x31, int32_t y31) {
    return getTileId(x31, y31, ZOOM_TO_LOAD_TILES_SHIFT_R);
}

int64_t OsmAnd::NetworkRouteContext_P::getTileId(int32_t x, int32_t y, int shiftR) {
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
        std::shared_ptr<const Road> road = *it;
        if (road == nullptr)
            continue;
        QVector<NetworkRouteKey> keys = convert(road);
        for (auto & rk : keys)
        {
            addToTile(osmcRoutesTile, road, rk);
        }
    }
    return osmcRoutesTile;
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteContext_P::convert(std::shared_ptr<const Road> road) const
{
    return filterKeys(getRouteKeys(road->getResolvedAttributes()));
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
                    QString key = tag.left(prefix.length() + 1);
                    addTag(routeKey, key, e.value());
                }
            }
            lst.append(routeKey);
        }
    }
    return lst;
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

int64_t OsmAnd::NetworkRouteContext_P::convertPointToLong(int x31, int y31)
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
        NetworkRoutePoint point = *(tile.routes.find(id));
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
