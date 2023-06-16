#ifndef _OSMAND_NETWORK_ROUTE_CONTEXT_P_H_
#define _OSMAND_NETWORK_ROUTE_CONTEXT_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>
#include <QHash>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "ObfRoutingSectionReader.h"
#include "NetworkRouteSelector.h"

#include <tuple>

namespace OsmAnd
{
class IObfsCollection;
class Road;
struct RoadInfo;

class NetworkRouteContext;
class NetworkRouteContext_P Q_DECL_FINAL
{
    Q_DISABLE_COPY_AND_MOVE(NetworkRouteContext_P);
   
protected:
    NetworkRouteContext_P(NetworkRouteContext* const owner);
public:
    ~NetworkRouteContext_P();
    ImplementationInterface<NetworkRouteContext> owner;
    
    friend class OsmAnd::NetworkRouteContext;
    
private:
    static const int ZOOM_TO_LOAD_TILES = 15;
    static const int ZOOM_TO_LOAD_TILES_SHIFT_L = ZOOM_TO_LOAD_TILES + 1;
    static const int ZOOM_TO_LOAD_TILES_SHIFT_R = 31 - ZOOM_TO_LOAD_TILES;
    
    struct NetworkRoutesTile
    {
        NetworkRoutesTile(int64_t tileId_):tileId(tileId_){};
        QMap<uint64_t, std::shared_ptr<NetworkRoutePoint>> routes;
        int64_t tileId;
        QHash<QString, std::shared_ptr<NetworkRouteSegment>> uniqueSegments;
        void add(const std::shared_ptr<const Road> road, NetworkRouteKey &routeKey);
        bool intersects(int x31, int y31, int px, int py) const;
        void addUnique(const std::shared_ptr<NetworkRouteSegment> &networkRouteSegment);
    };
    
    QHash<int64_t, NetworkRoutesTile> indexedTiles;
    
    QHash<NetworkRouteKey, QList< std::shared_ptr<NetworkRouteSegment>>> loadRouteSegmentsBbox(AreaI area, NetworkRouteKey * rKey);
    void loadRouteSegmentIntersectingTile(int32_t x, int32_t y, const NetworkRouteKey * routeKey,
                              QHash<NetworkRouteKey, QList<std::shared_ptr<NetworkRouteSegment>>> & map);
    NetworkRoutesTile getMapRouteTile(int32_t x31, int32_t y31);
    NetworkRoutesTile loadTile(int32_t x, int32_t y, int64_t tileId);
    
    static int64_t getTileId(int32_t x31, int32_t y31);
    static int64_t getTileId(int32_t x31, int32_t y31, int shiftR);
    static int32_t getXFromTileId(int64_t tileId);
    static int32_t getYFromTileId(int64_t tileId);
    static int64_t convertPointToLong(int x31, int y31);
    OsmAnd::PointI getPointFromLong(int64_t l) const;
    
    static void addObjectToPoint(const std::shared_ptr<NetworkRoutePoint> & point, const std::shared_ptr<const Road> road, NetworkRouteKey & routeKey, int start, int end);
    
    QVector<NetworkRouteKey> filterKeys(QVector<NetworkRouteKey> keys) const;
    QVector<NetworkRouteKey> convert(std::shared_ptr<const Road> & road) const;
};
}

#endif // !defined(_OSMAND_NETWORK_ROUTE_SELECTOR_P_H_)
