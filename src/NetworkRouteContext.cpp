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

const QString OsmAnd::NetworkRouteContext::ROUTE_KEY_VALUE_SEPARATOR = QStringLiteral("__");

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
    robj.get();
}

//OsmAnd::NetworkRouteKey::NetworkRouteKey()
//: type(static_cast<OsmRouteType>(OsmRouteType))
//{
//}

OsmAnd::NetworkRouteKey::NetworkRouteKey(NetworkRouteKey & other)
: type(other.type), tags(other.tags)
{
}

OsmAnd::NetworkRouteKey::NetworkRouteKey(const NetworkRouteKey & other)
: type(other.type), tags(other.tags)
{
}

//OsmAnd::NetworkRouteKey::NetworkRouteKey(int index)
//: type(static_cast<RouteType>(index))
//{
//}

OsmAnd::NetworkRouteKey::~NetworkRouteKey()
{
}

static const QVector<QString> ROUTE_TYPES_PREFIX { "route_hiking_", "route_bicycle_", "route_mtb_", "route_horse_"};
static const QVector<QString> ROUTE_TYPES_TAGS { "hiking", "bicycle", "mtb", "horse" };
const QString OsmAnd::NetworkRouteKey::NETWORK_ROUTE_TYPE = QStringLiteral("type");

QString OsmAnd::NetworkRouteKey::getTag() const
{
    return "hiking"; //ROUTE_TYPES_TAGS[static_cast<int>(type)];
}

void OsmAnd::NetworkRouteKey::addTag(const QString& key, const QString& value)
{
    QString val = value.isEmpty() ? QStringLiteral("") : NetworkRouteContext::ROUTE_KEY_VALUE_SEPARATOR + value;
    const QString tag = QStringLiteral("route_") + getTag() + NetworkRouteContext::ROUTE_KEY_VALUE_SEPARATOR + key + val;
    tags.insert(tag);
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteKey::getRouteKeys(const QHash<QString, QString> &tags)
{
    QVector<NetworkRouteKey> lst;
    for (const auto& tagPrefix : ROUTE_TYPES_PREFIX)
    {
        int rq = getRouteQuantity(tags, tagPrefix);
        for (int routeIdx = 1; routeIdx <= rq; routeIdx++)
        {
            const QString & prefix = tagPrefix + QString::number(routeIdx);
            NetworkRouteKey routeKey(ROUTE_TYPES_PREFIX.indexOf(tagPrefix));
            for (auto e = tags.begin(); e != tags.end(); ++e)
            {
                const QString & tag = e.key();
                if (tag.startsWith(prefix) && tag.length() > prefix.length())
                {
                    QString key = tag.right(tag.length() - (prefix.length() + 1));
                    routeKey.addTag(key, e.value());
                }
            }
            lst.append(routeKey);
        }
    }
    return lst;
}

QVector<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteKey::getRouteKeys(const std::shared_ptr<const Road> &road)
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

int OsmAnd::NetworkRouteKey::getRouteQuantity(const QHash<QString, QString>& tags, const QString& tagPrefix)
{
    int q = 0;
    for (auto e = tags.begin(); e != tags.end(); ++e)
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

QString OsmAnd::NetworkRouteKey::getNetwork() {
    return getValue(QStringLiteral("network"));
}

QString OsmAnd::NetworkRouteKey::getOperator() {
    return getValue(QStringLiteral("operator"));
}

QString OsmAnd::NetworkRouteKey::getSymbol() {
    return getValue(QStringLiteral("symbol"));
}

QString OsmAnd::NetworkRouteKey::getWebsite() {
    return getValue(QStringLiteral("website"));
}

QString OsmAnd::NetworkRouteKey::getWikipedia() {
    return getValue(QStringLiteral("wikipedia"));
}

QMap<QString, QString> OsmAnd::NetworkRouteKey::tagsToGpx() const
{
    QMap<QString, QString> networkRouteKey;
    networkRouteKey.insert(NETWORK_ROUTE_TYPE, getTag());
    for (auto & tag : tags)
    {
        QString key = getKeyFromTag(tag);
        QString value = getValue(key);
        if (!value.isEmpty())
        {
            networkRouteKey.insert(key, value);
        }
    }
    return networkRouteKey;
}

std::shared_ptr<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteKey::fromGpx(const QMap<QString, QString> & networkRouteKeyTags)
{
    auto it = networkRouteKeyTags.find(NETWORK_ROUTE_TYPE);
    if (it != networkRouteKeyTags.end())
    {
        QString type = *it;
        int rtype = ROUTE_TYPES_TAGS.indexOf(type);
//        if (rtype < static_cast<int>(OsmRouteType::Count))
//        {
//            const auto routeKey = std::make_shared<OsmAnd::NetworkRouteKey>(rtype);
//            for (auto i = networkRouteKeyTags.begin(); i != networkRouteKeyTags.end(); ++i)
//            {
//                routeKey->addTag(i.key(), i.value());
//            }
//            return routeKey;
//        }
    }
    return nullptr;
}

QString OsmAnd::NetworkRouteKey::getKeyFromTag(const QString& tag) const
{
    QString prefix = QStringLiteral("route_") + getTag() + NetworkRouteContext::ROUTE_KEY_VALUE_SEPARATOR;
    if (tag.startsWith(prefix) && tag.length() > prefix.length())
    {
        int endIdx = tag.indexOf(NetworkRouteContext::ROUTE_KEY_VALUE_SEPARATOR, prefix.length());
        return tag.mid(prefix.length(), endIdx - prefix.length());
    }
    return QStringLiteral("");
}

QString OsmAnd::NetworkRouteKey::getValue(const QString& key) const
{
    QString k = NetworkRouteContext::ROUTE_KEY_VALUE_SEPARATOR + key + NetworkRouteContext::ROUTE_KEY_VALUE_SEPARATOR;
    for (auto & tag : tags)
    {
        int i = tag.indexOf(k);
        if (i > 0)
        {
            return tag.right(tag.length() - (i + k.length()));
        }
    }
    return QStringLiteral("");
}

QString OsmAnd::NetworkRouteKey::getRouteName() const
{
    QString name = getValue(QStringLiteral("name"));
    if (name.isEmpty())
        name = getValue(QStringLiteral("ref"));
    if (name.isEmpty())
        name = getRelationID();

    return name;
}

QString OsmAnd::NetworkRouteKey::getRelationID() const
{
    return getValue(QStringLiteral("relation_id"));
}

OsmAnd::NetworkRoutePoint::~NetworkRoutePoint()
{    
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

QString OsmAnd::NetworkRouteKey::toString() const
{
    QString hash = QString((type.name.c_str()));
    for (auto & s : tags)
    {
        hash += s;
    }
    return hash;
}
