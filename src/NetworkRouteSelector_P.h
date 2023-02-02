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
    
private:
    
    
    
    
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
    
    QMap<NetworkRouteKey, std::shared_ptr<GpxDocument>> getRoutes(const AreaI area31, bool loadRoutes, NetworkRouteKey * routeKey) const;
    friend class OsmAnd::NetworkRouteSelector;
    
private:
    const int CONNECT_POINTS_DISTANCE_STEP = 50;
    const int CONNECT_POINTS_DISTANCE_MAX = 1000;
    struct NetworkRouteSegmentChain
    {
        NetworkRouteSegmentChain();
        NetworkRouteSegment start;
        QList<NetworkRouteSegment> connected;
        
        int getSize() const;
        NetworkRouteSegment getLast() const;
        PointI getEndPoint() const;
        void addChain(NetworkRouteSegmentChain & toAdd);
        void setStart(NetworkRouteSegment & newStart);
        void setEnd(NetworkRouteSegment & newEnd);
        inline bool operator == (const NetworkRouteSegmentChain & other) const
        {
            return connected.size() == other.connected.size()
                && start.routeKey == other.start.routeKey
                && start.start == other.start.start
                && start.end == other.start.end
                && getEndPoint() == other.getEndPoint();
        }
    };
    
    void connectAlgorithm(NetworkRouteSegment & segment, QMap<NetworkRouteKey, std::shared_ptr<GpxDocument>> & res);
    QList<NetworkRouteSegment> loadData(NetworkRouteSegment & segment, NetworkRouteKey & rkey);
    void addEnclosedTiles(QList<int64_t> queue, int64_t tileid);
    void debug(QString msg, short reverse, NetworkRouteSegment & segment) const;
    
    QList<NetworkRouteSegmentChain> getNetworkRouteSegmentChains(NetworkRouteKey & routeKey,
                                                                 QMap<NetworkRouteKey, std::shared_ptr<GpxDocument>> & res,
                                                                 const QList<NetworkRouteSegment> & loaded);
    QMap<int64_t, QList<NetworkRouteSegmentChain>> createChainStructure(const QList<NetworkRouteSegment> & lst);
    QMap<int64_t, QList<NetworkRouteSegmentChain>> prepareEndChain(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains);
    void add(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains, int64_t pnt, NetworkRouteSegmentChain & chain);
    void remove(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains, long pnt, NetworkRouteSegmentChain & toRemove);
    void chainAdd(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                  QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, NetworkRouteSegmentChain & it, NetworkRouteSegmentChain & toAdd);
    int connectSimpleMerge(QMap<int64_t, QList<NetworkRouteSegmentChain>> chains,
                           QMap<int64_t, QList<NetworkRouteSegmentChain>> endChains, int rad, int radE);
    int connectSimpleStraight(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                              QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad, int radE);
    int connectToLongestChain(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                              QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad);
    int reverseToConnectMore(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                             QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad, int radE);
    QList<NetworkRouteSegmentChain> filterChains(QList<NetworkRouteSegmentChain> & lst, NetworkRouteSegmentChain & ch, int rad, bool start);
    QList<NetworkRouteSegmentChain> flattenChainStructure(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains);
    QList<NetworkRouteSegmentChain> getByPoint(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains, long pnt,
                                               int radius, NetworkRouteSegmentChain & exclude);
    std::shared_ptr<GpxDocument> createGpxFile(QList<NetworkRouteSegmentChain> & chains, NetworkRouteKey & routeKey);
    
};
}

#endif // !defined(_OSMAND_NETWORK_ROUTE_SELECTOR_P_H_)
