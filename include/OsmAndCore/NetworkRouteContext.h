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
    //class Road;

    enum class RouteType
    {
        HIKING,
        BICYCLE,
        MTB,
        HORSE,
        
        Count
    };

    struct OSMAND_CORE_API NetworkRouteKey
    {
        NetworkRouteKey();
        NetworkRouteKey(NetworkRouteKey & other);
        NetworkRouteKey(const NetworkRouteKey & other);
        NetworkRouteKey(int index);
        virtual ~NetworkRouteKey();
        RouteType type;
        QSet<QString> tags;
        inline bool operator == (const NetworkRouteKey & other) const
        {
            return (type == other.type && tags == other.tags);
        }
        inline bool operator != (const NetworkRouteKey & other) const
        {
            return (type != other.type || tags != other.tags);
        }
        inline bool operator > (const NetworkRouteKey & other) const
        {
            return (tags.size() > other.tags.size());
        }
        inline bool operator < (const NetworkRouteKey & other) const
        {
            return (tags.size() < other.tags.size());
        }
        inline operator const QString() const
        {
            QString hash = QString::number(static_cast<int>(type));
            for (auto & s : tags)
            {
                hash += s;
            }
            return hash;
        }
    };

    struct OSMAND_CORE_API NetworkRouteSegment
    {
        NetworkRouteSegment();
        NetworkRouteSegment(NetworkRouteSegment & other);
        NetworkRouteSegment(const NetworkRouteSegment & other);
        NetworkRouteSegment(std::shared_ptr<const Road> road, NetworkRouteKey rKey, int start_, int end_);
        virtual ~NetworkRouteSegment();
        int start;
        int end;
        std::shared_ptr<const Road> robj;
        NetworkRouteKey routeKey;
        bool direction() { return end > start; };
    };

    struct OSMAND_CORE_API NetworkRouteSelectorFilter
    {
        NetworkRouteSelectorFilter();
        NetworkRouteSelectorFilter(NetworkRouteSelectorFilter & other);
        virtual ~NetworkRouteSelectorFilter();
        QSet<NetworkRouteKey> keyFilter;
        QSet<RouteType> typeFilter;
    };

    struct OSMAND_CORE_API NetworkRoutePoint
    {
        NetworkRoutePoint(int32_t x31, int32_t y31, int64_t id_): point(x31, y31), id(id_) {};
        virtual ~NetworkRoutePoint();
        PointI point;
        int64_t id;
        QVector<NetworkRouteSegment> objects;
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
            NetworkRouteSelectorFilter filter,
            const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache = nullptr
            );
        virtual ~NetworkRouteContext();

        const std::shared_ptr<const IObfsCollection> obfsCollection;
        const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache> cache;
        NetworkRouteSelectorFilter filter;
        
        QMap<NetworkRouteKey, QList<NetworkRouteSegment>> loadRouteSegmentsBbox(AreaI area, NetworkRouteKey * rKey);
        QVector<NetworkRouteKey> getRouteKeys(QHash<QString, QString> tags) const;
        int64_t getTileId(int32_t x31, int32_t y31) const;
        int64_t getTileId(int32_t x31, int32_t y31, int shiftR) const;
        void loadRouteSegmentTile(int32_t x, int32_t y, NetworkRouteKey * routeKey,
                                  QMap<NetworkRouteKey, QList<NetworkRouteSegment>> & map);
        int32_t getXFromTileId(int64_t tileId) const;
        int32_t getYFromTileId(int64_t tileId) const;
        PointI getPointFromLong(int64_t l) const;
        int64_t convertPointToLong(int x31, int y31) const;
        int64_t convertPointToLong(PointI point) const;
        QMap<QString, QString> tagsToGpx(const NetworkRouteKey & key) const;
        NetworkRouteKey * fromGpx(const QMap<QString, QString> & networkRouteKeyTags) const;
    };
}

#endif // !defined(_OSMAND_CORE_NETWORK_ROUTE_CONTEXT_H_)
