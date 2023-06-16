#ifndef _OSMAND_NETWORK_ROUTE_SELECTOR_P_H_
#define _OSMAND_NETWORK_ROUTE_SELECTOR_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>

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

class NetworkRouteSelector;
class NetworkRouteSelector_P Q_DECL_FINAL
{
    Q_DISABLE_COPY_AND_MOVE(NetworkRouteSelector_P);
public:
    typedef OsmAnd::NetworkRouteKey NetworkRouteKey;
    
protected:
    NetworkRouteSelector_P(NetworkRouteSelector* const owner);
public:
    ~NetworkRouteSelector_P();
    
    ImplementationInterface<NetworkRouteSelector> owner;
    
    QList<std::shared_ptr<const Road> > getRoutes(
                                                  const AreaI area31,
                                                  NetworkRouteKey * routeKey,
                                                  const RoutingDataLevel dataLevel,
                                                  QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>> * const outReferencedCacheEntries) const;
    
    QHash<NetworkRouteKey, std::shared_ptr<GpxDocument>> getRoutes(const AreaI area31, bool loadRoutes, NetworkRouteKey * routeKey) const;
    friend class OsmAnd::NetworkRouteSelector;
    
private:
    const int CONNECT_POINTS_DISTANCE_STEP = 50;
    const int CONNECT_POINTS_DISTANCE_MAX = 1000;
    struct NetworkRouteSegmentChain
    {
        NetworkRouteSegmentChain();
        std::shared_ptr<NetworkRouteSegment> start;
        QList<std::shared_ptr<NetworkRouteSegment>> connected;
        
        int getSize() const;
        std::shared_ptr<NetworkRouteSegment> getLast() const;
        PointI getStartPoint() const;
        PointI getEndPoint() const;
        void addChain(std::shared_ptr<NetworkRouteSegmentChain> & toAdd);
        void setStart(std::shared_ptr<NetworkRouteSegment> & newStart);
        void setEnd(std::shared_ptr<NetworkRouteSegment> & newEnd);
        inline bool operator == (const NetworkRouteSegmentChain & other) const
        {
            int64_t robjId = start->robj ? start->robj->id.id : 0;
            int64_t otherRobjId = other.start->robj ? other.start->robj->id.id : 0;
            return connected.size() == other.connected.size()
            && robjId == otherRobjId
            && start->start == other.start->start
            && start->end == other.start->end
            && getEndPoint() == other.getEndPoint()
            && getStartPoint() == other.getStartPoint()
            && start->routeKey == other.start->routeKey;
        };
        inline bool operator != (const NetworkRouteSegmentChain & other) const
        {
            int64_t robjId = start->robj ? start->robj->id.id : 0;
            int64_t otherRobjId = other.start->robj ? other.start->robj->id.id : 0;
            return connected.size() != other.connected.size()
            || robjId != otherRobjId
            || start->routeKey != other.start->routeKey
            || start->start != other.start->start
            || start->end != other.start->end
            || getEndPoint() != other.getEndPoint()
            || getStartPoint() != other.getStartPoint();
        }
    };
    
    void connectAlgorithm(const std::shared_ptr<NetworkRouteSegment> & segment, QHash<NetworkRouteKey, std::shared_ptr<GpxDocument>> & res) const;
    QList<std::shared_ptr<NetworkRouteSegment>> loadData(const std::shared_ptr<NetworkRouteSegment> & segment, const NetworkRouteKey & rkey) const;
    void addEnclosedTiles(QList<int64_t> & queue, int64_t tileid) const;
    void debug(QString msg, short reverse, const std::shared_ptr<NetworkRouteSegment> & segment) const;
    bool isCancelled() const;
    
    const QList<std::shared_ptr<NetworkRouteSegmentChain>> getNetworkRouteSegmentChains(const NetworkRouteKey & routeKey,
                                                                 QHash<NetworkRouteKey, std::shared_ptr<GpxDocument>> & res,
                                                                 const QList<std::shared_ptr<NetworkRouteSegment>> & loaded) const;
    QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> createChainStructure(const QList<std::shared_ptr<NetworkRouteSegment>> & lst) const;
    QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> prepareEndChain(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains) const;
    void add(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains, int64_t pnt, std::shared_ptr<NetworkRouteSegmentChain> & chain) const;
    void remove(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains, int64_t pnt, std::shared_ptr<NetworkRouteSegmentChain> & toRemove) const;
    void chainAdd(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains,
                  QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & endChains, std::shared_ptr<NetworkRouteSegmentChain> & it, std::shared_ptr<NetworkRouteSegmentChain> & toAdd) const;
    int connectSimpleMerge(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains,
                           QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & endChains, int rad, int radE) const;
    int connectSimpleStraight(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains,
                              QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & endChains, int rad, int radE) const;
    int connectToLongestChain(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains,
                              QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & endChains, int rad) const;
    int reverseToConnectMore(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains,
                             QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & endChains, int rad, int radE) const;
    QList<std::shared_ptr<NetworkRouteSegmentChain>> filterChains(QList<std::shared_ptr<NetworkRouteSegmentChain>> & lst, std::shared_ptr<NetworkRouteSegmentChain> & ch, int rad, bool start) const;
    const QList<std::shared_ptr<NetworkRouteSegmentChain>> flattenChainStructure(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains) const;
    QList<std::shared_ptr<NetworkRouteSegmentChain>> getByPoint(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains, int64_t pnt,
                                               int radius, const std::shared_ptr<NetworkRouteSegmentChain> &exclude) const;
    std::shared_ptr<NetworkRouteSegmentChain> chainReverse(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & chains,
                                          QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> & endChains,
                                          std::shared_ptr<NetworkRouteSegmentChain> & it) const;
    std::shared_ptr<NetworkRouteSegment> inverse(std::shared_ptr<NetworkRouteSegment> & seg) const;
    std::shared_ptr<GpxDocument> createGpxFile(const QList<std::shared_ptr<NetworkRouteSegmentChain>> & chains, const NetworkRouteKey & routeKey) const;
};
}

#endif // !defined(_OSMAND_NETWORK_ROUTE_SELECTOR_P_H_)
