#include "NetworkRouteSelector_P.h"
#include "NetworkRouteSelector.h"

#include <functional>
#include "Road.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"
#include "Utilities.h"
#include <OsmAndCore/Logging.h>
#include "commonOsmAndCore.h"

OsmAnd::NetworkRouteSelector_P::NetworkRouteSelector_P(NetworkRouteSelector* const owner_)
    : owner(owner_)
{
}


OsmAnd::NetworkRouteSelector_P::~NetworkRouteSelector_P()
{
}

OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::NetworkRouteSegmentChain()
{
}

QHash<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> OsmAnd::NetworkRouteSelector_P::getRoutes(const AreaI area31, bool loadRoutes, NetworkRouteKey * selected) const
{
    QHash<OsmAnd::NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> routeSegmentTile = owner->rCtx->loadRouteSegmentsBbox(area31, nullptr);
    
    /*for (auto i = routeSegmentTile.begin(); i != routeSegmentTile.end(); ++i)
    {
        auto & key = i.key();
        QString tags;
        for (const QString & tag : key.tags)
        {
            tags += " " + tag + "\n";
        }
        auto & v = i.value();
        QString s;
        for (auto & seg : v)
        {
            s += QString::number(seg.robj->id.id / 64) + ": " + QString::number(seg.start) + " " + QString::number(seg.end) + "\n";
        }
        OsmAnd::LogPrintf(LogSeverityLevel::Info, "Tags: \n %s Segments: \n %s", qPrintable(tags), qPrintable(s));
    }*/
    
    QHash<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> resultMap;
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
                connectAlgorithm(firstSegment, resultMap);
            }
        }
    }
    return resultMap;
}

void OsmAnd::NetworkRouteSelector_P::connectAlgorithm(OsmAnd::NetworkRouteSegment & segment,
                                                      QHash<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> & res) const
{
    NetworkRouteKey & rkey = segment.routeKey;
    debug("START ", 0, segment);
    const auto loaded = loadData(segment, rkey);
    QList<NetworkRouteSegmentChain> lst = getNetworkRouteSegmentChains(segment.routeKey, res, loaded);
    debug("FINISH " + QString::number(lst.size()), 0, segment);
}

void OsmAnd::NetworkRouteSelector_P::debug(QString msg, short direction, NetworkRouteSegment & segment) const
{
    QString routeKeyString;
    switch (segment.routeKey.type) {
        case RouteType::HORSE:
            routeKeyString = "HORSE";
            break;
        case RouteType::HIKING:
            routeKeyString = "HIKING";
            break;
        case RouteType::BICYCLE:
            routeKeyString = "BICYCLE";
            break;
        case RouteType::MTB:
            routeKeyString = "MTB";
            break;
        default:
            routeKeyString = "UNKNOWN";
    }
    for (const QString & tag : segment.routeKey.tags)
    {
        routeKeyString += " " + tag;
    }
    QString nrs = "NetworkRouteObject [start=" + QString::number(segment.start)
        + ", end=" + QString::number(segment.end)
        + ", obj=" + segment.robj->toString()
        + ", routeKeyHash=" + (int)segment.routeKey
        + ", routeKeyString=" + routeKeyString + "]";
    OsmAnd::LogPrintf(LogSeverityLevel::Info, "%s %s %s",
                      qPrintable(msg),
                      qPrintable(direction == 0 ? "" : (direction == -1 ? "-" : "+")),
                      qPrintable(nrs));
}

QList<OsmAnd::NetworkRouteSegment> OsmAnd::NetworkRouteSelector_P::loadData(NetworkRouteSegment & segment, NetworkRouteKey & rkey) const
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
        QHash<NetworkRouteKey, QList<OsmAnd::NetworkRouteSegment>> tiles;
        owner->rCtx->loadRouteSegmentTile(owner->rCtx->getXFromTileId(tileID), owner->rCtx->getYFromTileId(tileID), &rkey, tiles);
        auto it = tiles.find(rkey);
        if (it == tiles.end())
        {
            continue;
        }
        auto loaded = *it;
        // stop exploring if no route key even intersects tile (dont check loaded.size() == 0 special case)]
        for (NetworkRouteSegment & s : loaded)
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

void OsmAnd::NetworkRouteSelector_P::addEnclosedTiles(QList<int64_t> & queue, int64_t tileid) const
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
                                                                                                     QHash<NetworkRouteKey, std::shared_ptr<GpxDocument>> & res,
                                                                                                     const QList<NetworkRouteSegment> & loaded) const
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

QMap<int64_t, QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::createChainStructure(const QList<NetworkRouteSegment> & lst) const
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

void OsmAnd::NetworkRouteSelector_P::add(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains, int64_t pnt, NetworkRouteSegmentChain & chain) const
{
    QList<NetworkRouteSegmentChain> lst;
    auto it = chains.find(pnt);
    if (it != chains.end())
    {
        lst = it.value();
    }
    lst.append(chain);
    chains.insert(pnt, lst);
}

QMap<int64_t, QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::prepareEndChain(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains) const
{
    QMap<int64_t, QList<NetworkRouteSegmentChain>> endChains;
    for (auto & ch : chains.values())
    {
        for (auto & chain : ch)
        {
            add(endChains, owner->rCtx->convertPointToLong(chain.getEndPoint()), chain);
        }
    }
    return endChains;
}

int OsmAnd::NetworkRouteSelector_P::connectSimpleMerge(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                       QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad, int radE) const
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
                          QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad, int radE) const
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
                QList<NetworkRouteSegmentChain> connectNextLst = getByPoint(chains, pnt, radE, &it);
                connectNextLst = filterChains(connectNextLst, it, rad, true);
                QList<NetworkRouteSegmentChain> connectToEndLst = getByPoint(endChains, pnt, radE, &it);
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

int OsmAnd::NetworkRouteSelector_P::reverseToConnectMore(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                         QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad, int radE) const
{
    int reversed = 0;
    const QList<int64_t> & longPoints = chains.keys();
    for (const int64_t & startPnt : longPoints)
    {
        auto it = chains.find(startPnt);
        QList<NetworkRouteSegmentChain> vls = it.value();
        for (auto i = vls.begin(); i != vls.end(); ++i)
        {
            NetworkRouteSegmentChain it = *i;
            long pnt = owner->rCtx->convertPointToLong(it.getEndPoint().x, it.getEndPoint().y);
            // 1. reverse if 2 segments start from same point
            QList<NetworkRouteSegmentChain> startLst = getByPoint(chains, pnt, radE, nullptr);
            bool noStartFromEnd = filterChains(startLst, it, rad, true).size() == 0;
            bool reverse = (noStartFromEnd && vls.size() > 0);
            // 2. reverse 2 segments ends at same point
            QList<NetworkRouteSegmentChain> endLst = getByPoint(endChains, pnt, radE, nullptr);
            reverse |= i == vls.begin() && filterChains(endLst, it, rad, false).size() > 1 && noStartFromEnd;
            if (reverse)
            {
                OsmAnd::LogPrintf(LogSeverityLevel::Info, "PNT: %ld", startPnt);
                chainReverse(chains, endChains, it);
                reversed++;
                break;
            }
        }
    }
    return reversed;
}

OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain OsmAnd::NetworkRouteSelector_P::chainReverse(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                                      QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains,
                                      NetworkRouteSegmentChain & it) const
{
    int64_t startPnt = owner->rCtx->convertPointToLong(it.getStartPoint());
    int64_t pnt = owner->rCtx->convertPointToLong(it.getEndPoint());
    QList<NetworkRouteSegment> lst;
    lst.insert(0, inverse(it.start));
    if (it.connected.size() > 0)
    {
        for (NetworkRouteSegment s : it.connected) {
            lst.insert(0, inverse(s));
        }
    }
    remove(chains, startPnt, it);
    remove(endChains, pnt, it);
    NetworkRouteSegmentChain newChain;
    newChain.start = lst.at(0);
    lst.removeAt(0);
    newChain.connected = lst;
    add(chains, owner->rCtx->convertPointToLong(newChain.getStartPoint()), newChain);
    add(endChains, owner->rCtx->convertPointToLong(newChain.getEndPoint()), newChain);
    return newChain;
}

int OsmAnd::NetworkRouteSelector_P::connectToLongestChain(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                          QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains, int rad) const
{
    QList<NetworkRouteSegmentChain> chainsFlat;
    for (QList<NetworkRouteSegmentChain> ch : chains.values())
    {
        chainsFlat.append(ch);
    }
    std::sort(chainsFlat.begin(), chainsFlat.end(), [] (const NetworkRouteSegmentChain o1, const NetworkRouteSegmentChain o2) {
        return o1.getSize() < o2.getSize();
    });
    int mergedCount = 0;
    for(int i = 0; i < chainsFlat.size(); )
    {
        auto & first = chainsFlat[i];
        bool merged = false;
        for (int j = i + 1; j < chainsFlat.size() && !merged /*&& !isCancelled()*/; j++)
        {
            auto & second = chainsFlat[j];
            PointI firstStartPoint = first.getStartPoint();
            PointI firstEndPoint = first.getEndPoint();
            PointI secondStartPoint = second.getStartPoint();
            PointI secondEndPoint = second.getEndPoint();
            if (squareRootDist31(firstEndPoint.x, firstEndPoint.y, secondEndPoint.x, firstEndPoint.y) < rad)
            {
                auto secondReversed = chainReverse(chains, endChains, second);
                chainAdd(chains, endChains, first, secondReversed);
                chainsFlat.removeAt(j);
                merged = true;
            }
            else if (squareRootDist31(firstStartPoint.x, firstStartPoint.y, secondStartPoint.x, secondStartPoint.y) < rad)
            {
                auto firstReversed = chainReverse(chains, endChains, first);
                chainAdd(chains, endChains, firstReversed, second);
                chainsFlat.removeAt(j);
                chainsFlat[i] = firstReversed;//chainsFlat.set(i, firstReversed);
                merged = true;
            }
            else if (squareRootDist31(firstEndPoint.x, firstEndPoint.y, secondStartPoint.x, secondStartPoint.y) < rad)
            {
                chainAdd(chains, endChains, first, second);
                chainsFlat.removeAt(j);
                merged = true;
            }
            else if (squareRootDist31(secondEndPoint.x, secondEndPoint.y, firstStartPoint.x, firstStartPoint.y) < rad)
            {
                chainAdd(chains, endChains, second, first);
                chainsFlat.removeAt(i);
                merged = true;
            }
        }
        if (!merged)
        {
            i++;
        }
        else
        {
            i = 0; // start over
            mergedCount++;
        }
    }
    OsmAnd::LogPrintf(LogSeverityLevel::Info, "Connect longest alternative chains: %d (radius %d)", mergedCount, rad);
    return mergedCount;
}

QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain> OsmAnd::NetworkRouteSelector_P::filterChains(QList<NetworkRouteSegmentChain> & lst, NetworkRouteSegmentChain & ch, int rad, bool start) const
{
    if (lst.size() == 0)
    {
        return lst;
    }
    QMutableListIterator<NetworkRouteSegmentChain> it(lst);
    while (it.hasNext())
    {
        NetworkRouteSegmentChain chain = it.next();
        double min = rad + 1;
        NetworkRouteSegment s = start ? chain.start : chain.getLast();
        NetworkRouteSegment last = ch.getLast();
        for (int i = 0; i < s.robj->points31.size(); i++)
        {
            for (int j = 0; j < last.robj->points31.size(); j++)
            {
                PointI p1 = last.robj->points31.at(j);
                PointI p2 = s.robj->points31.at(i);
                double m = squareRootDist31(p1.x, p1.y, p2.x, p2.y);
                if (m < min)
                {
                    min = m;
                }
            }
        }
        if (min > rad)
        {
            it.remove();
        }
    }
    return lst;
}

QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain> OsmAnd::NetworkRouteSelector_P::flattenChainStructure(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains) const
{
    QList<NetworkRouteSegmentChain> chainsFlat;
    for (auto & ch : chains.values())
    {
        chainsFlat.append(ch);
    }
    std::sort(chainsFlat.begin(), chainsFlat.end(), [] (const NetworkRouteSegmentChain o1, const NetworkRouteSegmentChain o2) {
        return o1.getSize() < o2.getSize();//-Integer.compare(o1.getSize(), o2.getSize());
    });
    return chainsFlat;
}

QList<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain> OsmAnd::NetworkRouteSelector_P::getByPoint(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains, int64_t pnt, int radius, NetworkRouteSegmentChain * exclude) const
{
    QList<NetworkRouteSegmentChain> list;
    QList<NetworkRouteSegmentChain> emptyList;
    if (radius == 0)
    {
        auto it = chains.find(pnt);
        if (it != chains.end())
        {
            list = it.value();
            if (!exclude || !list.contains(*exclude))
            {
                QList copy(constOf(list));
                return copy;
            }
            else if (list.size() == 1)
            {
                list = emptyList;
            }
            else
            {
                QList copy(constOf(list));
                list = copy;
                list.removeOne(*exclude);
            }
        }
    }
    else
    {
        PointI point = owner->rCtx->getPointFromLong(pnt);
        QMapIterator<int64_t, QList<NetworkRouteSegmentChain>> it(chains);
        while (it.hasNext())
        {
            auto e = it.next();
            PointI point2 = owner->rCtx->getPointFromLong(e.key());
            if (squareRootDist31(point.x, point.y, point2.x, point2.y) < radius)
            {
                for (const NetworkRouteSegmentChain & c : e.value())
                {
                    if (!exclude || c != *exclude)
                    {
                        list.append(c);
                    }
                }
            }
        }
    }
    return list;
}

OsmAnd::NetworkRouteSegment OsmAnd::NetworkRouteSelector_P::inverse(NetworkRouteSegment & seg) const
{
    NetworkRouteSegment inversed(seg.robj, seg.routeKey, seg.end, seg.start);
    return inversed;
}

void OsmAnd::NetworkRouteSelector_P::remove(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains, int64_t pnt, NetworkRouteSegmentChain & toRemove) const
{
    auto it = chains.find(pnt);
    if (it == chains.end())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Error, "Point %d can not be remove from chains map", pnt);
    }
    else
    {
        QList<NetworkRouteSegmentChain> & lch = it.value();
        if (lch.removeOne(toRemove))
        {
            chains.remove(pnt);
        }
        else
        {
            OsmAnd::LogPrintf(LogSeverityLevel::Error, "Point %d can not be remove from chains map", pnt);
        }
    }
}

void OsmAnd::NetworkRouteSelector_P::chainAdd(QMap<int64_t, QList<NetworkRouteSegmentChain>> & chains,
                                              QMap<int64_t, QList<NetworkRouteSegmentChain>> & endChains,
                                              NetworkRouteSegmentChain & it,
                                              NetworkRouteSegmentChain & toAdd) const
{
    if (it == toAdd) {
        OsmAnd::LogPrintf(LogSeverityLevel::Error, "Chain can not be added. Chains are equals");
    }
    remove(chains, owner->rCtx->convertPointToLong(toAdd.getStartPoint()), toAdd);
    remove(endChains, owner->rCtx->convertPointToLong(toAdd.getEndPoint()), toAdd);
    remove(endChains, owner->rCtx->convertPointToLong(it.getEndPoint()), it);
    PointI currentEnd = it.getEndPoint();
    PointI toAddStart = toAdd.getStartPoint();
    double minStartDist = squareRootDist31(currentEnd.x, currentEnd.y, toAddStart.x, toAddStart.y);
    double minLastDist = minStartDist;
    int minStartInd = toAdd.start.start;
    auto & points31 = toAdd.start.robj->points31;
    for (int i = 0; i < points31.size(); i++)
    {
        PointI point = points31.at(i);
        double m = squareRootDist31(currentEnd.x, currentEnd.y, point.x, point.y);
        if (m < minStartDist && minStartInd != i)
        {
            minStartInd = i;
            minStartDist = m;
        }
    }
    NetworkRouteSegment lastIt = it.getLast();
    int minLastInd = lastIt.end;
    auto & lastItPoints31 = lastIt.robj->points31;
    for (int i = 0; i < lastItPoints31.size(); i++)
    {
        PointI point = lastItPoints31.at(i);
        double m = squareRootDist31(point.x, point.y, toAddStart.x, toAddStart.y);
        if (m < minLastDist && minLastInd != i)
        {
            minLastInd = i;
            minLastDist = m;
        }
    }
    if (minLastDist > minStartDist)
    {
        if (minStartInd != toAdd.start.start)
        {
            NetworkRouteSegment newSeg(toAdd.start.robj, toAdd.start.routeKey, minStartInd, toAdd.start.end);
            toAdd.setStart(newSeg);
        }
    }
    else
    {
        if (minLastInd != lastIt.end)
        {
            NetworkRouteSegment newSeg(lastIt.robj, lastIt.routeKey, lastIt.start, minLastInd);
            it.setEnd(newSeg);
        }
    }
    it.addChain(toAdd);
    add(endChains, owner->rCtx->convertPointToLong(currentEnd), it);
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::NetworkRouteSelector_P::createGpxFile(QList<NetworkRouteSegmentChain> & chains, NetworkRouteKey & routeKey) const
{
    std::shared_ptr<GpxDocument> gpxFile = std::make_shared<GpxDocument>();
    std::shared_ptr<GpxDocument::Track> track = std::make_shared<GpxDocument::Track>();
    QList<int> sizes;
    for (const auto & c : constOf(chains)) {
        QList<NetworkRouteSegment> segmentList;
        segmentList.append(c.start);
        if (c.connected.size() > 0) {
            segmentList.append(c.connected);
        }
        
        std::shared_ptr<GpxDocument::TrkSegment> trkSegment = make_shared<GpxDocument::TrkSegment>();
        track->segments.append(trkSegment);
        int l = 0;
        std::shared_ptr<GpxDocument::WptPt> prev;
        for (NetworkRouteSegment & segment : segmentList)
        {
            QVector<float> heightArray;
            if (segment.robj)
            {
                //no heights in Road object, how to convert to RouteDataObject
                //heightArray = segment.robj->calculateHeightArray();
            }
            int inc = segment.start < segment.end ? 1 : -1;
            for (int i = segment.start; ; i += inc)
            {
                std::shared_ptr<GpxDocument::WptPt> point = make_shared<GpxDocument::WptPt>();
                PointI p = segment.robj->points31.at(i);
                point->position.latitude = OsmAnd::Utilities::get31LatitudeY(p.y);
                point->position.longitude = OsmAnd::Utilities::get31LongitudeX(p.x);
                if (heightArray.size() > i * 2 + 1)
                {
                    point->elevation = heightArray[i * 2 + 1];
                }
                trkSegment->points.append(point);
                if (prev)
                {
                    l += getDistance(prev->position.latitude, prev->position.longitude, point->position.latitude, point->position.longitude);
                }
                prev = point;
                if (i == segment.end) {
                    break;
                }
            }
        }
        sizes.append(l);
    }
    QString log = "";
    for (int s : sizes)
    {
        log += QString::number(s);
    }
    OsmAnd::LogPrintf(LogSeverityLevel::Info, "Segments size %d: %s", track->segments.size(), qPrintable(log));
    gpxFile->tracks.append(track);
    gpxFile->networkRouteKeyTags = owner->rCtx->tagsToGpx(routeKey);
    return gpxFile;
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
    const NetworkRouteSegment & seg = getLast();
    if (seg.robj && seg.robj->points31.size() > seg.end)
    {
        return seg.robj->points31[seg.end];
    }
    PointI def(0, 0);
    return def;
}

OsmAnd::PointI OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::getStartPoint() const
{
    return start.robj->points31[start.start];
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
