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
        static const OsmRouteType* WATER;
        static const OsmRouteType* WINTER;
        static const OsmRouteType* SNOWMOBILE;
        static const OsmRouteType* RIDING;
        static const OsmRouteType* RACING;
        static const OsmRouteType* MOUNTAINBIKE;
        static const OsmRouteType* BICYCLE;
        static const OsmRouteType* MTB;
        static const OsmRouteType* CYCLING;
        static const OsmRouteType* HIKING;
        static const OsmRouteType* RUNNING;
        static const OsmRouteType* WALKING;
        static const OsmRouteType* OFFROAD;
        static const OsmRouteType* MOTORBIKE;
        static const OsmRouteType* DIRTBIKE;
        static const OsmRouteType* CAR;
        static const OsmRouteType* HORSE;
        static const OsmRouteType* ROAD;
        static const OsmRouteType* DETOUR;
        static const OsmRouteType* BUS;
        static const OsmRouteType* CANOE;
        static const OsmRouteType* FERRY;
        static const OsmRouteType* FOOT;
        static const OsmRouteType* LIGHT_RAIL;
        static const OsmRouteType* PISTE;
        static const OsmRouteType* RAILWAY;
        static const OsmRouteType* SKI;
        static const OsmRouteType* ALPINE;
        static const OsmRouteType* FITNESS;
        static const OsmRouteType* INLINE_SKATES;
        static const OsmRouteType* SUBWAY;
        static const OsmRouteType* TRAIN;
        static const OsmRouteType* TRACKS;
        static const OsmRouteType* TRAM;
        static const OsmRouteType* TROLLEYBUS;
        static const OsmRouteType* CLIMBING;

        // OsmRouteType.UNKNOWN is used for TravelGpx OSM routes.
        // It allows us to reuse code of RouteInfoCard, RouteKey icons, etc.
        static const OsmRouteType* UNKNOWN;

        const QString name;
        const QString renderingPropertyAttr;
        const QString tagPrefix;
        const QString color;
        const QString icon;

        OsmRouteType(const QString& name, const QString& renderingPropertyAttr = QString(),
                     const QString& color = QString(), const QString& icon = QString());

    public:
        static const OsmRouteType* getByTag(const QString& tag);
        static const OsmRouteType* getByRenderingPropertyAttr(const QString& renderingPropertyAttr);

        inline bool operator == (const OsmRouteType& other) const
        {
            return (this->name == other.name &&
                    this->color == other.color &&
                    this->icon == other.icon);
        }

        inline bool operator != (const OsmRouteType& other) const
        {
            return !(* this == other);
        }
    };

    inline uint qHash(const OsmRouteType& key, uint seed = 0) {
        return qHash(key.name) + qHash(key.color) + qHash(key.icon);
    }

    struct OSMAND_CORE_API NetworkRouteKey
    {
        NetworkRouteKey();
        NetworkRouteKey(const NetworkRouteKey& other);
        NetworkRouteKey(const OsmRouteType* type);
        virtual ~NetworkRouteKey();
        const OsmRouteType* type;
        QSet<QString> tags;
        QString toString() const;
        
        static QVector<NetworkRouteKey> getRouteKeys(const QHash<QString, QString>& tags);
        static QVector<NetworkRouteKey> getRouteKeys(const std::shared_ptr<const Road> &road);

        static bool containsUnsupportedRouteTags(const QHash<QString, QString>& tags);

        static int getRouteQuantity(const QHash<QString, QString>& tags, const QString& tagPrefix);
        QString getTag() const;
        QMap<QString, QString> tagsMap() const;
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
            if (type)
            	result = prime * result + qHash(*type);

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
        void setNetworkFilter(NetworkRouteSelectorFilter & selectorFilter);
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
