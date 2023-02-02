#include "NetworkRouteSelector_P.h"
#include "NetworkRouteSelector.h"

#include "Road.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"
#include "Utilities.h"
#include <OsmAndCore/Logging.h>

OsmAnd::NetworkRouteSelector_P::NetworkRouteSelector_P(NetworkRouteSelector* const owner_)
    : owner(owner_)
{
}


OsmAnd::NetworkRouteSelector_P::~NetworkRouteSelector_P()
{
}

QMap<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> OsmAnd::NetworkRouteSelector_P::getRoutes(const AreaI area31, bool loadRoutes, NetworkRouteKey * selected) const
{
    QMap<OsmAnd::NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> routeSegmentTile = owner->rCtx->loadRouteSegmentsBbox(area31, selected);
    QMap<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> resultMap;
    for (auto i = routeSegmentTile.begin(); i != routeSegmentTile.end(); ++i)
    {
        auto & routeKey = i.key();
        if (selected != nullptr && *selected != routeKey)
        {
            continue;
        }
        auto & routeSegments = i.value();
        if (routeSegmentTile.size() > 0)
        {
            if (!loadRoutes)
            {
                resultMap.insert(routeKey, nullptr);
            }
            else
            {
                NetworkRouteSegment firstSegment = routeSegments.at(0);
                //connectAlgorithm
            }
        }
    }
    return resultMap;
}

void OsmAnd::NetworkRouteSelector_P::connectAlgorithm(OsmAnd::NetworkRouteSegment & segment,
                                                      QMap<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> & res)
{
    NetworkRouteKey & rkey = segment.routeKey;
    debug("START ", 0, segment);
    const auto loaded = loadData(segment, rkey);
    //QList<NetworkRouteSegmentChain> lst = getNetworkRouteSegmentChains(segment.routeKey, res, loaded);
    //debug("FINISH " + QString::number(lst.size()), 0, segment);
}

void OsmAnd::NetworkRouteSelector_P::debug(QString msg, short direction, NetworkRouteSegment & segment) const
{
    QString nrs = "NetworkRouteObject [start=" + QString::number(segment.start)
        + ", end=" + QString::number(segment.end)
        + ", obj=" + segment.robj->toString()
        + ", routeKey=" + (QString)segment.routeKey + "]";
    OsmAnd::LogPrintf(LogSeverityLevel::Info, "%s %s %s",
                      qPrintable(msg),
                      qPrintable(direction == 0 ? "" : (direction == -1 ? "-" : "+")),
                      qPrintable(nrs));
}

QList<OsmAnd::NetworkRouteSegment> OsmAnd::NetworkRouteSelector_P::loadData(NetworkRouteSegment & segment, NetworkRouteKey & rkey)
{
    
    QList<OsmAnd::NetworkRouteSegment> result;
    if (!segment.robj)
    {
        return result;
    }
    QList<int64_t> queue;
    QSet<int64_t> visitedTiles;
    QSet<int64_t> objIds;
    int64_t start = owner->rCtx->getTileId(segment.robj->points31[segment.start].x, segment.robj->points31[segment.start].y);
    int64_t end = owner->rCtx->getTileId(segment.robj->points31[segment.end].x, segment.robj->points31[segment.end].y);
    queue.append(start);
    queue.append(end);
    while (!queue.isEmpty() /*&& !isCancelled()*/)
    {
        int64_t tileID = queue.at(queue.size() - 1);
        queue.removeAt(queue.size() - 1);
        if (visitedTiles.contains(tileID))
        {
            continue;
        }
        visitedTiles.insert(tileID);
        QMap<NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> tiles;
        owner->rCtx->loadRouteSegmentTile(owner->rCtx->getXFromTileId(tileID), owner->rCtx->getYFromTileId(tileID), &rkey, tiles);
        auto it = tiles.find(rkey);
        if (it == tiles.end())
        {
            continue;
        }
        auto loaded = *it;
        // stop exploring if no route key even intersects tile (dont check loaded.size() == 0 special case)
        for (NetworkRouteSegment s : loaded)
        {
            if (!objIds.contains(s.robj->id))
            {
                objIds.insert(s.robj->id);
                result.append(s);
            }
        }
        addEnclosedTiles(queue, tileID);
    }
    return result;
}

void OsmAnd::NetworkRouteSelector_P::addEnclosedTiles(QList<int64_t> queue, int64_t tileid)
{
    int x = owner->rCtx->getXFromTileId(tileid);
    int y = owner->rCtx->getYFromTileId(tileid);
    for (int dx = -1; dx <= 1; dx++)
    {
        for (int dy = -1; dy <= 1; dy++)
        {
            if (!(dy == 0 && dx == 0))
            {
                queue.append(owner->rCtx->getTileId(x + dx, y + dy, 0));
            }
        }
    }
}

QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain> OsmAnd::NetworkRouteSelector_P::getNetworkRouteSegmentChains(NetworkRouteKey & routeKey,
                                                                                                     QMap<NetworkRouteKey, std::shared_ptr<GpxDocument>> & res,
                                                                                                     const QList<NetworkRouteSegment> & loaded)
{
    OsmAnd::LogPrintf(LogSeverityLevel::Info, "About to merge: %d", loaded.size());
    QMap<int64_t, QList<NetworkRouteSegmentChain>> chains = createChainStructure(loaded);
    QMap<int64_t, QList<NetworkRouteSegmentChain>> endChains = prepareEndChain(chains);
    connectSimpleMerge(chains, endChains, 0, 0);
    connectSimpleMerge(chains, endChains, 0, CONNECT_POINTS_DISTANCE_STEP);
    for (int s = 0; s < CONNECT_POINTS_DISTANCE_MAX; s += CONNECT_POINTS_DISTANCE_STEP)
    {
        connectSimpleMerge(chains, endChains, s, s + CONNECT_POINTS_DISTANCE_STEP);
    }
    connectToLongestChain(chains, endChains, CONNECT_POINTS_DISTANCE_STEP);
    connectSimpleMerge(chains, endChains, 0, CONNECT_POINTS_DISTANCE_STEP);
    connectSimpleMerge(chains, endChains, CONNECT_POINTS_DISTANCE_MAX / 2, CONNECT_POINTS_DISTANCE_MAX);
    QList<NetworkRouteSegmentChain> lst = flattenChainStructure(chains);
    std::shared_ptr<GpxDocument> gpxFile = createGpxFile(lst, routeKey);
    res.insert(routeKey, gpxFile);
    return lst;
}

QMap<int64_t, QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::createChainStructure(const QList<NetworkRouteSegment> & lst)
{
    QMap<int64_t, QList<NetworkRouteSegmentChain>> chains;
    for (NetworkRouteSegment s : lst)
    {
        NetworkRouteSegmentChain chain;
        chain.start = s;
        int64_t pnt = owner->rCtx->convertPointToLong(s.robj->points31[s.start].x, s.robj->points31[s.start].y);
        add(chains, pnt, chain);
    }
    return chains;
}

void OsmAnd::NetworkRouteSelector_P::add(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains, int64_t pnt, NetworkRouteSegmentChain & chain)
{
    auto it = chains.find(pnt);
    if (it == chains.end())
    {
        QList<NetworkRouteSegmentChain> lst;
        chains.insert(pnt, lst);
    }
}

QMap<int64_t, QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::prepareEndChain(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains)
{
    QMap<int64_t, QList<NetworkRouteSegmentChain>> endChains;
    for (auto & ch : chains.values())
    {
        for (auto & chain : ch)
        {
            auto endPoint = chain.getEndPoint();
            add(endChains, owner->rCtx->convertPointToLong(endPoint.x, endPoint.y), chain);
        }
    }
    return endChains;
}

int OsmAnd::NetworkRouteSelector_P::connectSimpleMerge(QMap<int64_t, QList<NetworkRouteSegmentChain>> chains,
                       QMap<int64_t, QList<NetworkRouteSegmentChain>> endChains, int rad, int radE)
{
    int merged = 1;
    while (merged > 0 /*&& !isCancelled()*/)
    {
        int rs = reverseToConnectMore(chains, endChains, rad, radE);
        merged = connectSimpleStraight(chains, endChains, rad, radE);
        OsmAnd::LogPrintf(LogSeverityLevel::Info, "Simple merged: %d, reversed: %d (radius %d %d)", merged, rs, rad, radE);
    }
    return merged;
}

int OsmAnd::NetworkRouteSelector_P::connectSimpleStraight(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                          QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad, int radE)
{
    int merged = 0;
    bool changed = true;
    while (changed /*&& !isCancelled()*/)
    {
        changed = false;
        mainLoop : for (auto & lst : chains.values())
        {
            for (NetworkRouteSegmentChain & it : lst)
            {
                auto endPoint = it.getEndPoint();
                int64_t pnt = owner->rCtx->convertPointToLong(endPoint.x, endPoint.y);
                QList<NetworkRouteSegmentChain> connectNextLst = getByPoint(chains, pnt, radE, it);
                connectNextLst = filterChains(connectNextLst, it, rad, true);
                QList<NetworkRouteSegmentChain> connectToEndLst = getByPoint(endChains, pnt, radE, it); // equal to c
                connectToEndLst = filterChains(connectToEndLst, it, rad, false);
                if (connectToEndLst.size() > 0)
                {
                    for (auto & next : connectNextLst)
                    {
                        connectToEndLst.removeAll(next);
                    }
                }
                // no alternative join
                if (connectNextLst.size() == 1 && connectToEndLst.size() == 0)
                {
                    //OsmAnd::LogPrintf(LogSeverityLevel::Info, " Merged: %d -> %d", it.getLast().robj->id / 128, connectNextLst.at(0).start.getId() / 128);
                    chainAdd(chains, endChains, it, connectNextLst[0]);
                    changed = true;
                    merged++;
                    goto mainLoop;
                }
            }
        }
    }
    return merged;
}

QList<std::shared_ptr<const OsmAnd::Road>> OsmAnd::NetworkRouteSelector_P::getRoutes(
    const AreaI area31,
    NetworkRouteKey * routeKey,
    const RoutingDataLevel dataLevel,
    QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>>* const outReferencedCacheEntries) const
{
    QList<std::shared_ptr<const Road>> result;

    const auto obfDataInterface = owner->rCtx->obfsCollection->obtainDataInterface(
        &area31,
        MinZoomLevel,
        MaxZoomLevel,
        ObfDataTypesMask().set(ObfDataType::Routing));
    ObfRoutingSectionReader::VisitorFunction visitor = [this, routeKey](const std::shared_ptr<const OsmAnd::Road>& road)
    {
        if (!routeKey)
        {
            return true;
        }
        QHash<QString, QString> tags = road->getResolvedAttributes();
        const auto & routeKeys = owner->rCtx->getRouteKeys(tags);
        if (routeKeys.contains(*routeKey))
        {
            return true;
        }
        return false;
    };
    obfDataInterface->loadRoads(
        dataLevel,
        &area31,
        &result,
        nullptr,
        visitor,
        owner->rCtx->cache.get(),
        outReferencedCacheEntries,
        nullptr,
        nullptr);

    return result;
}

int OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::getSize() const
{
    return 1 + connected.size();
}

OsmAnd::NetworkRouteSegment OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::getLast() const
{
    if (connected.size() > 0)
    {
        return connected.at(connected.size() - 1);
    }
    return start;
}

OsmAnd::PointI OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::getEndPoint() const
{
    NetworkRouteSegment const & seg = getLast();
    if (seg.robj)
    {
        return seg.robj->points31[seg.end];
    }
    PointI def(0, 0);
    return def;
}

void OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::addChain(NetworkRouteSegmentChain & toAdd)
{
    connected.append(toAdd.start);
    connected.append(toAdd.connected);
}

void OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::setStart(NetworkRouteSegment & newStart)
{
    start = newStart;
}

void OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::setEnd(NetworkRouteSegment & newEnd)
{
    if (connected.size() > 0)
    {
        connected.removeAt(connected.size() - 1);
        connected.append(newEnd);
    }
    else
    {
        start = newEnd;
    }
}
