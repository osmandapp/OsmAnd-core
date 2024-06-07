#ifndef _OSMAND_CORE_NETWORK_ROUTE_CONTEXT_H_
#define _OSMAND_CORE_NETWORK_ROUTE_CONTEXT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/Data/Road.h>

namespace OsmAnd
{
    class IObfsCollection;

    class OSMAND_CORE_API OsmRouteType
    {
    public:
        static OsmRouteType WATER;
        static OsmRouteType WINTER;
        static OsmRouteType SNOWMOBILE;
        static OsmRouteType RIDING;
        static OsmRouteType RACING;
        static OsmRouteType MOUNTAINBIKE;
        static OsmRouteType BICYCLE;
        static OsmRouteType MTB;
        static OsmRouteType CYCLING;
        static OsmRouteType HIKING;
        static OsmRouteType RUNNING;
        static OsmRouteType WALKING;
        static OsmRouteType OFFROAD;
        static OsmRouteType MOTORBIKE;
        static OsmRouteType CAR;
        static OsmRouteType HORSE;
        static OsmRouteType ROAD;
        static OsmRouteType DETOUR;
        static OsmRouteType BUS;
        static OsmRouteType CANOE;
        static OsmRouteType FERRY;
        static OsmRouteType FOOT;
        static OsmRouteType LIGHT_RAIL;
        static OsmRouteType PISTE;
        static OsmRouteType RAILWAY;
        static OsmRouteType SKI;
        static OsmRouteType ALPINE;
        static OsmRouteType FITNESS;
        static OsmRouteType INLINE_SKATES;
        static OsmRouteType SUBWAY;
        static OsmRouteType TRAIN;
        static OsmRouteType TRACKS;
        static OsmRouteType TRAM;
        static OsmRouteType TROLLEYBUS;

        static QList<OsmRouteType> values;
        QString name;
        QString tagPrefix;
        QString color;
        QString icon;
        
        class RouteActivityTypeBuilder
        {
        public:
            OsmRouteType* routeType;
            RouteActivityTypeBuilder();
            OsmRouteType reg();
            RouteActivityTypeBuilder & color(const QString & color);
            RouteActivityTypeBuilder & icon(const QString & icon);
        };

        OsmRouteType();
        OsmRouteType(QString n);
        OsmRouteType(const OsmRouteType & ort);

        static std::shared_ptr<OsmRouteType> getByTag(QString tag);

        inline bool operator == (const OsmRouteType & other) const
        {
            return (this->name == other.name &&
                    this->color == other.color &&
                    this->icon == other.icon);
        }

        inline bool operator != (const OsmRouteType & other) const
        {
            return !(* this == other);
        }
    private:
        static RouteActivityTypeBuilder createType(const QString & name);
    };

    inline uint qHash(const OsmRouteType & key, uint seed = 0) {
        return qHash(key.name) + qHash(key.color) + qHash(key.icon);
    }

    struct OSMAND_CORE_API NetworkRouteKey
    {
        NetworkRouteKey();
        NetworkRouteKey(NetworkRouteKey & other);
        NetworkRouteKey(const NetworkRouteKey & other);
        NetworkRouteKey(OsmRouteType ort);
        virtual ~NetworkRouteKey();
        OsmRouteType type;
        QSet<QString> tags;
        QString toString() const;
        
        static QVector<NetworkRouteKey> getRouteKeys(const QHash<QString, QString>& tags);
        static QVector<NetworkRouteKey> getRouteKeys(const std::shared_ptr<const Road> &road);
        
        static int getRouteQuantity(const QHash<QString, QString>& tags, const QString& tagPrefix);
        QString getTag() const;
        QMap<QString, QString> tagsToGpx() const;
        static std::shared_ptr<NetworkRouteKey> fromGpx(const QMap<QString, QString> &networkRouteKeyTags);
        QString getKeyFromTag(const QString& tag) const;
        QString getValue(const QString& key) const;
        QString getRouteName() const;
        QString getRelationID() const;

        QString getNetwork();
        QString getOperator();
        QString getSymbol();
        QString getWebsite();
        QString getWikipedia();
        
        void addTag(const QString& key, const QString& value);
        
        inline bool operator == (const NetworkRouteKey & other) const
        {
            if (type != other.type)
                return false;
            if (tags.size() != other.tags.size())
                return false;
            bool tagsEqual = true;
            for (auto & tag : tags)
            {
                if (!other.tags.contains(tag))
                {
                    tagsEqual = false;
                    break;
                }
            }
            return tagsEqual;
        }
        inline bool operator != (const NetworkRouteKey & other) const
        {
            if (type != other.type)
                return false;
            if (tags.size() != other.tags.size())
                return false;
            bool tagsNotEqual = false;
            for (auto & tag : tags)
            {
                if (!other.tags.contains(tag))
                {
                    tagsNotEqual = true;
                    break;
                }
            }
            return tagsNotEqual;
        }

        inline bool operator > (const NetworkRouteKey & other) const
        {
            return (type == other.type || tags.size() > other.tags.size());
        }
        inline bool operator < (const NetworkRouteKey & other) const
        {
            return (type == other.type || tags.size() < other.tags.size());
        }
        inline operator int() const
        {
            const int prime = 31;
            int result = 1;
            int s = 0;
            for (const QString & tag : constOf(tags))
            {
                s += qHash(tag);
            }
            result = prime * result + s;
            result = prime * result + qHash(type);
            return result;
        }
    private:
        static const QString NETWORK_ROUTE_TYPE;
    };

    struct OSMAND_CORE_API NetworkRouteSegment
    {
        NetworkRouteSegment();
        NetworkRouteSegment(NetworkRouteSegment & other);
        NetworkRouteSegment(const NetworkRouteSegment & other);
        NetworkRouteSegment(const std::shared_ptr<const Road> & road, const NetworkRouteKey & rKey, int start_, int end_);
        virtual ~NetworkRouteSegment();
        const int start;
        const int end;
        const std::shared_ptr<const Road> robj;
        const NetworkRouteKey routeKey;
        bool direction() { return end > start; };
    };

    struct OSMAND_CORE_API NetworkRouteSelectorFilter
    {
        NetworkRouteSelectorFilter();
        NetworkRouteSelectorFilter(NetworkRouteSelectorFilter & other);
        virtual ~NetworkRouteSelectorFilter();
        QSet<NetworkRouteKey> keyFilter;
        QSet<OsmRouteType> typeFilter;
    };

    struct OSMAND_CORE_API NetworkRoutePoint
    {
        NetworkRoutePoint(int32_t x31, int32_t y31, int64_t id_): point(x31, y31), id(id_) {};
        virtual ~NetworkRoutePoint();
        const PointI point;
        const int64_t id;
        QVector<std::shared_ptr<NetworkRouteSegment>> objects;
        double localVar;
    };

    class NetworkRouteContext_P;
    class OSMAND_CORE_API NetworkRouteContext
    {
        Q_DISABLE_COPY_AND_MOVE(NetworkRouteContext);
    private:
        PrivateImplementation<NetworkRouteContext_P> _p;
    public:
        NetworkRouteContext(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache = nullptr
            );
        virtual ~NetworkRouteContext();
        
        static const QString ROUTE_KEY_VALUE_SEPARATOR;

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache> cache;
        NetworkRouteSelectorFilter filter;
        
        void setNetworkRouteKeyFilter(NetworkRouteKey & routeKey);
        QHash<NetworkRouteKey, QList<std::shared_ptr<NetworkRouteSegment>>> loadRouteSegmentsBbox(AreaI area, NetworkRouteKey * rKey);
        int64_t getTileId(int32_t x31, int32_t y31) const;
        int64_t getTileId(int32_t x31, int32_t y31, int shiftR) const;
        void loadRouteSegmentIntersectingTile(int32_t x, int32_t y, const NetworkRouteKey * routeKey,
                                  QHash<NetworkRouteKey, QList<std::shared_ptr<NetworkRouteSegment>>> & map);
        int32_t getXFromTileId(int64_t tileId) const;
        int32_t getYFromTileId(int64_t tileId) const;
        PointI getPointFromLong(int64_t l) const;
        int64_t convertPointToLong(int x31, int y31) const;
        int64_t convertPointToLong(PointI point) const;
    };
}

#endif // !defined(_OSMAND_CORE_NETWORK_ROUTE_CONTEXT_H_)
