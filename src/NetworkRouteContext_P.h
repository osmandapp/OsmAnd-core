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
    const int ZOOM_TO_LOAD_TILES = 15;
    const int ZOOM_TO_LOAD_TILES_SHIFT_L = ZOOM_TO_LOAD_TILES + 1;
    const int ZOOM_TO_LOAD_TILES_SHIFT_R = 31 - ZOOM_TO_LOAD_TILES;
    const QString ROUTE_KEY_VALUE_SEPARATOR { "__" };
    const QString NETWORK_ROUTE_TYPE = "type";
    const QVector<QString> ROUTE_TYPES_TAGS { "hiking", "bicycle", "mtb", "horse" };
    const QVector<QString> ROUTE_TYPES_PREFIX { "route_hiking_", "route_bicycle_", "route_mtb_", "route_horse_"};
    
    struct NetworkRoutesTile
    {
        NetworkRoutesTile(int64_t tileId_):tileId(tileId_){};
        QMap<uint64_t, NetworkRoutePoint> routes;
        int64_t tileId;
    };
    
    QHash<int64_t, NetworkRoutesTile> indexedTiles;
    
    QMap<NetworkRouteKey, QList<NetworkRouteSegment>> loadRouteSegmentsBbox(AreaI area, NetworkRouteKey * rKey);
    void loadRouteSegmentTile(int32_t x, int32_t y, NetworkRouteKey * routeKey,
                              QMap<NetworkRouteKey, QList<NetworkRouteSegment>> & map);
    NetworkRoutesTile getMapRouteTile(int32_t x31, int32_t y31);
    NetworkRoutesTile loadTile(int32_t x, int32_t y, int64_t tileId);
    void addToTile(NetworkRoutesTile & tile, std::shared_ptr<const Road> road, NetworkRouteKey & routeKey);
    
    QVector<NetworkRouteKey> getRouteKeys(QHash<QString, QString> tags) const;
    int getRouteQuantity(QHash<QString, QString> & tags, const QString tagPrefix) const;
    int64_t getTileId(int32_t x31, int32_t y31) const;
    int64_t getTileId(int32_t x31, int32_t y31, int shiftR) const;
    int32_t getXFromTileId(int64_t tileId) const;
    int32_t getYFromTileId(int64_t tileId) const;
    int64_t convertPointToLong(int x31, int y31) const;
    OsmAnd::PointI getPointFromLong(int64_t l) const;

    void addObjectToPoint(NetworkRoutePoint & point, std::shared_ptr<const Road> road, NetworkRouteKey & routeKey, int start, int end) const;
    
    QVector<NetworkRouteKey> filterKeys(QVector<NetworkRouteKey> keys) const;
    QVector<NetworkRouteKey> convert(std::shared_ptr<const Road> & road) const;
    QString getTag(NetworkRouteKey & routeKey) const;
    void addTag(NetworkRouteKey & routeKey, QString key, QString value) const;
    QMap<QString, QString> tagsToGpx(const NetworkRouteKey & key) const;
    NetworkRouteKey * fromGpx(const QMap<QString, QString> & networkRouteKeyTags) const;
    QString getKeyFromTag(const NetworkRouteKey & key, QString tag) const;
    QString getValue(const NetworkRouteKey & routeKey, QString key) const;
};
}

#endif // !defined(_OSMAND_NETWORK_ROUTE_SELECTOR_P_H_)
