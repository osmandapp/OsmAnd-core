#include "NetworkRouteContext.h"
#include "NetworkRouteContext_P.h"

static QList<const OsmAnd::OsmRouteType*> VALUES;

static const OsmAnd::OsmRouteType _WATER("water", "whiteWaterSports", "yellow", "special_kayak");
static const OsmAnd::OsmRouteType _WINTER("winter", "", "yellow", "special_skiing");
static const OsmAnd::OsmRouteType _SNOWMOBILE("snowmobile", "", "yellow", "special_snowmobile");
static const OsmAnd::OsmRouteType _RIDING("riding", "", "yellow", "special_horse");
static const OsmAnd::OsmRouteType _RACING("racing", "", "yellow", "raceway");
static const OsmAnd::OsmRouteType _MOUNTAINBIKE("mountainbike", "", "blue", "sport_cycling");
static const OsmAnd::OsmRouteType _BICYCLE("bicycle", "showCycleRoutes");
static const OsmAnd::OsmRouteType _MTB("mtb", "showMtbRoutes");
static const OsmAnd::OsmRouteType _CYCLING("cycling", "", "blue", "special_bicycle");
static const OsmAnd::OsmRouteType _HIKING("hiking", "hikingRoutesOSMC", "orange", "special_trekking");
static const OsmAnd::OsmRouteType _RUNNING("running", "showRunningRoutes", "orange", "running");
static const OsmAnd::OsmRouteType _WALKING("walking", "", "orange", "special_walking");
static const OsmAnd::OsmRouteType _OFFROAD("offroad", "", "yellow", "special_offroad");
static const OsmAnd::OsmRouteType _MOTORBIKE("motorbike", "", "green", "special_motorcycle");
static const OsmAnd::OsmRouteType _DIRTBIKE("dirtbike", "showDirtbikeTrails");
static const OsmAnd::OsmRouteType _CAR("car", "", "green", "shop_car");
static const OsmAnd::OsmRouteType _HORSE("horse", "horseRoutes");
static const OsmAnd::OsmRouteType _ROAD("road");
static const OsmAnd::OsmRouteType _DETOUR("detour");
static const OsmAnd::OsmRouteType _BUS("bus");
static const OsmAnd::OsmRouteType _CANOE("canoe");
static const OsmAnd::OsmRouteType _FERRY("ferry");
static const OsmAnd::OsmRouteType _FOOT("foot");
static const OsmAnd::OsmRouteType _LIGHT_RAIL("light_rail");
static const OsmAnd::OsmRouteType _PISTE("piste", "pisteRoutes");
static const OsmAnd::OsmRouteType _RAILWAY("railway");
static const OsmAnd::OsmRouteType _SKI("ski");
static const OsmAnd::OsmRouteType _ALPINE("alpine", "alpineHiking");
static const OsmAnd::OsmRouteType _FITNESS("fitness_trail", "showFitnessTrails");
static const OsmAnd::OsmRouteType _INLINE_SKATES("inline_skates");
static const OsmAnd::OsmRouteType _SUBWAY("subway");
static const OsmAnd::OsmRouteType _TRAIN("train");
static const OsmAnd::OsmRouteType _TRACKS("tracks");
static const OsmAnd::OsmRouteType _TRAM("tram");
static const OsmAnd::OsmRouteType _TROLLEYBUS("trolleybus");
static const OsmAnd::OsmRouteType _CLIMBING("climbing", "showClimbingRoutes");
static const OsmAnd::OsmRouteType _UNKNOWN("unknown");

const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::WATER = &_WATER;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::WINTER = &_WINTER;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::SNOWMOBILE = &_SNOWMOBILE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::RIDING = &_RIDING;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::RACING = &_RACING;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::MOUNTAINBIKE = &_MOUNTAINBIKE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::BICYCLE = &_BICYCLE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::MTB = &_MTB;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::CYCLING = &_CYCLING;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::HIKING = &_HIKING;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::RUNNING = &_RUNNING;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::WALKING = &_WALKING;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::OFFROAD = &_OFFROAD;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::MOTORBIKE = &_MOTORBIKE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::DIRTBIKE = &_DIRTBIKE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::CAR = &_CAR;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::HORSE = &_HORSE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::ROAD = &_ROAD;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::DETOUR = &_DETOUR;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::BUS = &_BUS;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::CANOE = &_CANOE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::FERRY = &_FERRY;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::FOOT = &_FOOT;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::LIGHT_RAIL = &_LIGHT_RAIL;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::PISTE = &_PISTE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::RAILWAY = &_RAILWAY;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::SKI = &_SKI;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::ALPINE = &_ALPINE;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::FITNESS = &_FITNESS;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::INLINE_SKATES = &_INLINE_SKATES;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::SUBWAY = &_SUBWAY;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::TRAIN = &_TRAIN;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::TRACKS = &_TRACKS;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::TRAM = &_TRAM;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::TROLLEYBUS = &_TROLLEYBUS;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::CLIMBING = &_CLIMBING;
const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::UNKNOWN = &_UNKNOWN;

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

OsmAnd::NetworkRouteSegment::NetworkRouteSegment() : start(0), end(0)
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


OsmAnd::NetworkRouteKey::NetworkRouteKey() : type(nullptr)
{
}

OsmAnd::NetworkRouteKey::NetworkRouteKey(const NetworkRouteKey& other) : type(other.type), tags(other.tags)
{
}

OsmAnd::NetworkRouteKey::NetworkRouteKey(const OsmRouteType* type_) : type(type_)
{
}

OsmAnd::NetworkRouteKey::~NetworkRouteKey()
{
}

const QString OsmAnd::NetworkRouteKey::NETWORK_ROUTE_TYPE = QStringLiteral("type");

QString OsmAnd::NetworkRouteKey::getTag() const
{
    return type ? type->name : QString();
}

OsmAnd::OsmRouteType::OsmRouteType(const QString& name, const QString& renderingPropertyAttr, const QString& color, const QString& icon)
    : name(name), renderingPropertyAttr(renderingPropertyAttr), tagPrefix(QStringLiteral("route_") + name + QStringLiteral("_")), color(color), icon(icon)
{
    VALUES.push_back(this);
}

const OsmAnd::OsmRouteType* OsmAnd::OsmRouteType::getByTag(const QString& tag)
{
    for (const auto routeType : VALUES)
        if (routeType->name == tag)
            return routeType;

    return nullptr;
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
    for(const auto routeType : VALUES)
    {
        if (routeType->renderingPropertyAttr.isEmpty())
        {
            continue; // unsupported
        }
        int rq = getRouteQuantity(tags, routeType->tagPrefix);
        for (int routeIdx = 1; routeIdx <= rq; routeIdx++)
        {
            const QString & prefix = routeType->tagPrefix + QString::number(routeIdx);
            NetworkRouteKey routeKey(routeType);
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

bool OsmAnd::NetworkRouteKey::containsUnsupportedRouteTags(const QHash<QString, QString>& tags)
{
    for (const auto routeType : VALUES)
    {
        if (routeType->renderingPropertyAttr.isEmpty() &&
            (tags.contains(QStringLiteral("route_") + routeType->name)
                || tags.contains(QStringLiteral("route_") + routeType->name + QStringLiteral("_1"))))
        {
            return true;
        }
    }
    return false;
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
                q = std::max(q, num);
        }
        
    }
    return q;
}

QString OsmAnd::NetworkRouteKey::getNetwork() 
{
    return getValue(QStringLiteral("network"));
}

QString OsmAnd::NetworkRouteKey::getOperator() 
{
    return getValue(QStringLiteral("operator"));
}

QString OsmAnd::NetworkRouteKey::getSymbol() 
{
    return getValue(QStringLiteral("symbol"));
}

QString OsmAnd::NetworkRouteKey::getWebsite() 
{
    return getValue(QStringLiteral("website"));
}

QString OsmAnd::NetworkRouteKey::getWikipedia() 
{
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
            networkRouteKey.insert(key, value);
    }
    return networkRouteKey;
}

std::shared_ptr<OsmAnd::NetworkRouteKey> OsmAnd::NetworkRouteKey::fromGpx(const QMap<QString, QString> & networkRouteKeyTags)
{
    auto it = networkRouteKeyTags.find(NETWORK_ROUTE_TYPE);
    if (it != networkRouteKeyTags.end())
    {
        QString type = *it;
        auto routeType = OsmRouteType::getByTag(type);
        if (routeType) 
        {
            const auto routeKey = std::make_shared<NetworkRouteKey>(routeType);
            for (auto i = networkRouteKeyTags.begin(); i != networkRouteKeyTags.end(); ++i)
                routeKey->addTag(i.key(), i.value());

            return routeKey;
        }
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
            return tag.right(tag.length() - (i + k.length()));
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
    QString res = type ? type->name : QString();
    for (auto & s : tags)
        res += s;

    return res;
}
