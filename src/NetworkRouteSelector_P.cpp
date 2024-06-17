#include "NetworkRouteSelector_P.h"
#include "NetworkRouteSelector.h"

#include <functional>
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

OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::NetworkRouteSegmentChain()
{
}

QHash<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> OsmAnd::NetworkRouteSelector_P::getRoutes(const AreaI area31, bool loadRoutes, NetworkRouteKey * selected) const
{
    auto routeSegmentTile = owner->rCtx->loadRouteSegmentsBbox(area31, nullptr);
    QHash<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> resultMap;
    for (auto i = routeSegmentTile.begin(); i != routeSegmentTile.end(); ++i)
    {
        auto &routeKey = i.key();
        if (selected != nullptr && *selected != routeKey)
        {
            continue;
        }
        auto &routeSegments = i.value();
        if (routeSegmentTile.size() > 0)
        {
            if (!loadRoutes)
            {
                resultMap.insert(routeKey, nullptr);
            }
            else
            {
                const auto &firstSegment = routeSegments.at(0);
                connectAlgorithm(firstSegment, resultMap);
            }
        }
    }
    return resultMap;
}

void OsmAnd::NetworkRouteSelector_P::connectAlgorithm(const std::shared_ptr<OsmAnd::NetworkRouteSegment> &segment,
                                                      QHash<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> &res) const
{
    const auto &rkey = segment->routeKey;
    debug(QStringLiteral("START "), 0, segment);
    const auto loaded = loadData(segment, rkey);
    const auto lst = getNetworkRouteSegmentChains(segment->routeKey, res, loaded);
    debug(QStringLiteral("FINISH ") + QString::number(lst.size()), 0, segment);
}

void OsmAnd::NetworkRouteSelector_P::debug(QString msg, short direction, const std::shared_ptr<OsmAnd::NetworkRouteSegment> &segment) const
{
    QString routeKeyString(segment->routeKey.type->name);
    for (const QString &tag : segment->routeKey.tags)
    {
        routeKeyString += QStringLiteral(" ") + tag;
    }
    QString nrs = QStringLiteral("NetworkRouteObject [start=") + QString::number(segment->start)
        + QStringLiteral(", end=") + QString::number(segment->end)
        + QStringLiteral(", obj=") + segment->robj->toString()
        + QStringLiteral(", routeKeyHash=") + (int)segment->routeKey
        + QStringLiteral(", routeKeyString=") + routeKeyString + "]";
    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "%s %s %s",
                      qPrintable(msg),
                      qPrintable(direction == 0 ? "" : (direction == -1 ? "-" : "+")),
                      qPrintable(nrs));
}

bool OsmAnd::NetworkRouteSelector_P::isCancelled() const
{
    return owner->queryController && owner->queryController->isAborted();
}

QList<std::shared_ptr<OsmAnd::NetworkRouteSegment>> OsmAnd::NetworkRouteSelector_P::loadData(const std::shared_ptr<NetworkRouteSegment> &segment, const NetworkRouteKey &rkey) const
{
    QList<std::shared_ptr<NetworkRouteSegment>> result;
    if (!segment->robj)
    {
        return result;
    }
    QList<int64_t> queue;
    QSet<int64_t> visitedTiles;
    QSet<int64_t> objIds;
    int64_t start = owner->rCtx->getTileId(segment->robj->points31[segment->start].x, segment->robj->points31[segment->start].y);
    int64_t end = owner->rCtx->getTileId(segment->robj->points31[segment->end].x, segment->robj->points31[segment->end].y);
    queue.append(start);
    queue.append(end);
    while (!queue.isEmpty() && !isCancelled())
    {
        int64_t tileID = queue.at(queue.size() - 1);
        queue.removeAt(queue.size() - 1);
        if (visitedTiles.contains(tileID))
        {
            continue;
        }
        visitedTiles.insert(tileID);
        QHash<NetworkRouteKey, QList<std::shared_ptr<OsmAnd::NetworkRouteSegment>>> tiles;
        int32_t xTile = owner->rCtx->getXFromTileId(tileID);
        int32_t yTile = owner->rCtx->getYFromTileId(tileID);
        owner->rCtx->loadRouteSegmentIntersectingTile(xTile, yTile, &rkey, tiles);
        auto it = tiles.find(rkey);
        if (it == tiles.end())
        {
            continue;
        }
        auto loaded = *it;
        // stop exploring if no route key even intersects tile (dont check loaded.size() == 0 special case)]
        for (auto &loadedSegment : loaded)
        {
            if (!objIds.contains(loadedSegment->robj->id.id))
            {
                objIds.insert(loadedSegment->robj->id.id);
                result.append(loadedSegment);
            }
        }
        addEnclosedTiles(queue, tileID);
    }
    return result;
}

void OsmAnd::NetworkRouteSelector_P::addEnclosedTiles(QList<int64_t> &queue, int64_t tileid) const
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

const QList<std::shared_ptr<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::getNetworkRouteSegmentChains(const NetworkRouteKey &routeKey,
                                                                                                     QHash<NetworkRouteKey, std::shared_ptr<GpxDocument>> &res,
                                                                                                     const QList<std::shared_ptr<NetworkRouteSegment>> &loaded) const
{
    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "About to merge: %d", loaded.size());
    QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> chains = createChainStructure(loaded);
    QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> endChains = prepareEndChain(chains);
    
    connectSimpleMerge(chains, endChains, 0, 0);
    connectSimpleMerge(chains, endChains, 0, CONNECT_POINTS_DISTANCE_STEP);
    for (int s = 0; s < CONNECT_POINTS_DISTANCE_MAX; s += CONNECT_POINTS_DISTANCE_STEP)
    {
        connectSimpleMerge(chains, endChains, s, s + CONNECT_POINTS_DISTANCE_STEP);
    }
    connectToLongestChain(chains, endChains, CONNECT_POINTS_DISTANCE_STEP);
    connectSimpleMerge(chains, endChains, 0, CONNECT_POINTS_DISTANCE_STEP);
    connectSimpleMerge(chains, endChains, CONNECT_POINTS_DISTANCE_MAX / 2, CONNECT_POINTS_DISTANCE_MAX);
    const auto lst = flattenChainStructure(chains);
    const auto gpxFile = createGpxFile(lst, routeKey);
    
    res.insert(routeKey, gpxFile);
    return lst;
}

QMap<int64_t, QList<std::shared_ptr<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>>> OsmAnd::NetworkRouteSelector_P::createChainStructure(const QList<std::shared_ptr<NetworkRouteSegment>> &lst) const
{
    QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> chains;
    for (const auto &s : lst)
    {
        auto chain = std::make_shared<NetworkRouteSegmentChain>();
        chain->start = s;
        int64_t pnt = owner->rCtx->convertPointToLong(s->robj->points31[s->start].x, s->robj->points31[s->start].y);
        add(chains, pnt, chain);
    }
    return chains;
}

void OsmAnd::NetworkRouteSelector_P::add(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains, int64_t pnt, std::shared_ptr<NetworkRouteSegmentChain> &chain) const
{
    auto it = chains.find(pnt);
    if (it == chains.end())
    {
        QList<std::shared_ptr<NetworkRouteSegmentChain>> emptyLst;
        it = chains.insert(pnt, emptyLst);
    }
    (*it).append(chain);
}

QMap<int64_t, QList<std::shared_ptr<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>>> OsmAnd::NetworkRouteSelector_P::prepareEndChain(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains) const
{
    QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> endChains;
    for (auto it = chains.begin(); it != chains.end(); ++it)
    {
        auto &ch = *it;
        for (auto &chain : ch)
        {
            add(endChains, owner->rCtx->convertPointToLong(chain->getEndPoint()), chain);
        }
    }
    return endChains;
}

int OsmAnd::NetworkRouteSelector_P::connectSimpleMerge(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains,
                       QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &endChains, int rad, int radE) const
{
    int merged = 1;
    while (merged > 0 && !isCancelled())
    {
        int rs = reverseToConnectMore(chains, endChains, rad, radE);
        merged = connectSimpleStraight(chains, endChains, rad, radE);
        OsmAnd::LogPrintf(LogSeverityLevel::Debug, "Simple merged: %d, reversed: %d (radius %d %d)", merged, rs, rad, radE);
    }
    return merged;
}

int OsmAnd::NetworkRouteSelector_P::connectSimpleStraight(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains,
                          QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &endChains, int rad, int radE) const
{
    int merged = 0;
    bool changed = true;
    while (changed && !isCancelled())
    {
        changed = false;
        for (auto it = chains.begin(); it != chains.end(); ++it)
        {
            if (changed)
            {
                break;
            }
            auto &lst = it.value();
            for (auto &chain : lst)
            {
                auto endPoint = chain->getEndPoint();
                int64_t pnt = owner->rCtx->convertPointToLong(endPoint.x, endPoint.y);
                QList<std::shared_ptr<NetworkRouteSegmentChain>> connectNextLst = getByPoint(chains, pnt, radE, chain);
                connectNextLst = filterChains(connectNextLst, chain, rad, true);
                QList<std::shared_ptr<NetworkRouteSegmentChain>> connectToEndLst = getByPoint(endChains, pnt, radE, chain);
                connectToEndLst = filterChains(connectToEndLst, chain, rad, false);
                if (connectToEndLst.size() > 0)
                {
                    for (auto &next : connectNextLst)
                    {
                        connectToEndLst.removeAll(next);
                    }
                }
                // no alternative join
                if (connectNextLst.size() == 1 && connectToEndLst.size() == 0)
                {
                    std::shared_ptr<NetworkRouteSegmentChain> &toAdd = connectNextLst[0];
                    chainAdd(chains, endChains, chain, toAdd);
                    changed = true;
                    merged++;
                    break;
                }
            }
        }
    }
    return merged;
}

int OsmAnd::NetworkRouteSelector_P::reverseToConnectMore(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains,
                         QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &endChains, int rad, int radE) const
{
    int reversed = 0;
    //chains.values() - copy only, chains is changing inside
    const auto keys = chains.keys();
    for (int64_t key : keys)
    {
        auto vls = chains[key];
        for (int i = 0; i < vls.count(); i++)
        {
            std::shared_ptr<NetworkRouteSegmentChain> &it = vls[i];
            long pnt = owner->rCtx->convertPointToLong(it->getEndPoint());
            // 1. reverse if 2 segments start from same point
            QList<std::shared_ptr<NetworkRouteSegmentChain>> startLst = getByPoint(chains, pnt, radE, nullptr);
            bool noStartFromEnd = filterChains(startLst, it, rad, true).size() == 0;
            bool reverse = (noStartFromEnd && vls.size() > 0);
            // 2. reverse 2 segments ends at same point
            QList<std::shared_ptr<NetworkRouteSegmentChain>> endLst = getByPoint(endChains, pnt, radE, nullptr);
            reverse |= i == 0 && filterChains(endLst, it, rad, false).size() > 1 && noStartFromEnd;
            if (reverse)
            {
                chainReverse(chains, endChains, it);
                reversed++;
                break;
            }
        }
    }
    return reversed;
}

std::shared_ptr<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain> OsmAnd::NetworkRouteSelector_P::chainReverse(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains,
                                      QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &endChains,
                                      std::shared_ptr<NetworkRouteSegmentChain> &it) const
{
    int64_t startPnt = owner->rCtx->convertPointToLong(it->getStartPoint());
    int64_t pnt = owner->rCtx->convertPointToLong(it->getEndPoint());
    QList<std::shared_ptr<NetworkRouteSegment>> lst;
    lst.insert(0, inverse(it->start));
    if (it->connected.size() > 0)
    {
        for (auto &s : it->connected) {
            lst.insert(0, inverse(s));
        }
    }
    remove(chains, startPnt, it);
    remove(endChains, pnt, it);
    auto newChain = std::make_shared<NetworkRouteSegmentChain>();
    newChain->start = lst.at(0);
    lst.removeAt(0);
    newChain->connected = lst;
    add(chains, owner->rCtx->convertPointToLong(newChain->getStartPoint()), newChain);
    add(endChains, owner->rCtx->convertPointToLong(newChain->getEndPoint()), newChain);
    return newChain;
}

int OsmAnd::NetworkRouteSelector_P::connectToLongestChain(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains,
                          QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &endChains, int rad) const
{
    QList<std::shared_ptr<NetworkRouteSegmentChain>> chainsFlat;
    for (auto i = chains.begin(); i != chains.end(); ++i)
    {
        auto &ch = i.value();
        chainsFlat.append(ch);
    }
    std::sort(chainsFlat.begin(), chainsFlat.end(), [] (const std::shared_ptr<NetworkRouteSegmentChain> o1, const std::shared_ptr<NetworkRouteSegmentChain> o2) {
        return o1->getSize() < o2->getSize();
    });
    int mergedCount = 0;
    for(int i = 0; i < chainsFlat.size(); )
    {
        auto &first = chainsFlat[i];
        bool merged = false;
        for (int j = i + 1; j < chainsFlat.size() && !merged && !isCancelled(); j++)
        {
            auto &second = chainsFlat[j];
            if (OsmAnd::Utilities::distance31(first->getEndPoint().x, first->getEndPoint().y, second->getEndPoint().x, second->getEndPoint().y) < rad)
            {
                auto secondReversed = chainReverse(chains, endChains, second);
                chainAdd(chains, endChains, first, secondReversed);
                chainsFlat.removeAt(j);
                merged = true;
            }
            else if (OsmAnd::Utilities::distance31(first->getStartPoint().x, first->getStartPoint().y, second->getStartPoint().x, second->getStartPoint().y) < rad)
            {
                auto firstReversed = chainReverse(chains, endChains, first);
                chainAdd(chains, endChains, firstReversed, second);
                chainsFlat.removeAt(j);
                chainsFlat[i] = firstReversed;//chainsFlat.set(i, firstReversed);
                merged = true;
            }
            else if (OsmAnd::Utilities::distance31(first->getEndPoint().x, first->getEndPoint().y, second->getStartPoint().x, second->getStartPoint().y) < rad)
            {
                chainAdd(chains, endChains, first, second);
                chainsFlat.removeAt(j);
                merged = true;
            }
            else if (OsmAnd::Utilities::distance31(second->getEndPoint().x, second->getEndPoint().y, first->getStartPoint().x, first->getStartPoint().y) < rad)
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
    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "Connect longest alternative chains: %d (radius %d)", mergedCount, rad);
    return mergedCount;
}

QList<std::shared_ptr<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::filterChains(QList<std::shared_ptr<NetworkRouteSegmentChain>> &lst, std::shared_ptr<NetworkRouteSegmentChain> &ch, int rad, bool start) const
{
    if (lst.size() == 0)
    {
        return lst;
    }
    QMutableListIterator<std::shared_ptr<NetworkRouteSegmentChain>> it(lst);
    while (it.hasNext())
    {
        std::shared_ptr<NetworkRouteSegmentChain> &chain = it.next();
        double min = rad + 1;
        const auto &s = start ? chain->start : chain->getLast();
        const auto &last = ch->getLast();
        for (int i = 0; i < s->robj->points31.size(); i++)
        {
            for (int j = 0; j < last->robj->points31.size(); j++)
            {
                const PointI p1 = last->robj->points31.at(j);
                const PointI p2 = s->robj->points31.at(i);
                double m = OsmAnd::Utilities::distance31(p1.x, p1.y, p2.x, p2.y);
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

const QList<std::shared_ptr<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::flattenChainStructure(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains) const
{
    QList<std::shared_ptr<NetworkRouteSegmentChain>> chainsFlat;
    for (auto i = chains.begin(); i != chains.end(); ++i)
    {
        auto &ch = i.value();
        chainsFlat.append(ch);
    }
    std::sort(chainsFlat.begin(), chainsFlat.end(), [] (const std::shared_ptr<NetworkRouteSegmentChain> o1, const std::shared_ptr<NetworkRouteSegmentChain> o2) {
        return o1->getSize() < o2->getSize();//-Integer.compare(o1.getSize(), o2.getSize());
    });
    return chainsFlat;
}

QList<std::shared_ptr<OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain>> OsmAnd::NetworkRouteSelector_P::getByPoint(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains, int64_t pnt, int radius, const std::shared_ptr<NetworkRouteSegmentChain> &exclude) const
{
    QList<std::shared_ptr<NetworkRouteSegmentChain>> list;
    QList<std::shared_ptr<NetworkRouteSegmentChain>> emptyList;
    if (radius == 0)
    {
        auto it = chains.find(pnt);
        if (it != chains.end())
        {
            list = it.value();
            if (!exclude || !list.contains(exclude))
            {
                return list;
            }
            else if (list.size() == 1)
            {
                list = emptyList;
            }
            else
            {
                list.removeOne(exclude);
            }
        }
    }
    else
    {
        PointI point = owner->rCtx->getPointFromLong(pnt);
        QMapIterator<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> it(chains);
        while (it.hasNext())
        {
            auto e = it.next();
            PointI point2 = owner->rCtx->getPointFromLong(e.key());
            if (OsmAnd::Utilities::distance31(point.x, point.y, point2.x, point2.y) < radius)
            {
                for (const std::shared_ptr<NetworkRouteSegmentChain> &c : e.value())
                {
                    if (!exclude || c != exclude)
                    {
                        list.append(c);
                    }
                }
            }
        }
    }
    return list;
}

std::shared_ptr<OsmAnd::NetworkRouteSegment> OsmAnd::NetworkRouteSelector_P::inverse(std::shared_ptr<NetworkRouteSegment> &seg) const
{
    std::shared_ptr<NetworkRouteSegment> inversed = std::make_shared<NetworkRouteSegment>(seg->robj, seg->routeKey, seg->end, seg->start);
    return inversed;
}

void OsmAnd::NetworkRouteSelector_P::remove(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains, int64_t pnt, std::shared_ptr<NetworkRouteSegmentChain> &toRemove) const
{
    auto it = chains.find(pnt);
    if (it == chains.end())
    {
        OsmAnd::LogPrintf(LogSeverityLevel::Error, "Can not remove point %ld from chains map", pnt);
    }
    else
    {
        auto &lch = it.value();
        if (!lch.removeOne(toRemove))
        {
            OsmAnd::LogPrintf(LogSeverityLevel::Error, "Remove segment chain from list. Point");
        }
        if (lch.isEmpty())
        {
            chains.remove(pnt);
        }
    }
}

void OsmAnd::NetworkRouteSelector_P::chainAdd(QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &chains,
                                              QMap<int64_t, QList<std::shared_ptr<NetworkRouteSegmentChain>>> &endChains,
                                              std::shared_ptr<NetworkRouteSegmentChain> &it,
                                              std::shared_ptr<NetworkRouteSegmentChain> &toAdd) const
{
    if (it == toAdd) {
        OsmAnd::LogPrintf(LogSeverityLevel::Error, "Chain can not be added. Chains are equals");
    }
    remove(chains, owner->rCtx->convertPointToLong(toAdd->getStartPoint()), toAdd);
    remove(endChains, owner->rCtx->convertPointToLong(toAdd->getEndPoint()), toAdd);
    remove(endChains, owner->rCtx->convertPointToLong(it->getEndPoint()), it);
    double minStartDist = OsmAnd::Utilities::distance31(it->getEndPoint().x, it->getEndPoint().y, toAdd->getStartPoint().x, toAdd->getStartPoint().y);
    double minLastDist = minStartDist;
    int minStartInd = toAdd->start->start;
    auto &points31 = toAdd->start->robj->points31;
    for (int i = 0; i < points31.size(); i++)
    {
        double m = OsmAnd::Utilities::distance31(it->getEndPoint().x, it->getEndPoint().y, points31.at(i).x, points31.at(i).y);
        if (m < minStartDist && minStartInd != i)
        {
            minStartInd = i;
            minStartDist = m;
        }
    }
    const auto &lastIt = it->getLast();
    int minLastInd = lastIt->end;
    auto &lastItPoints31 = lastIt->robj->points31;
    for (int i = 0; i < lastItPoints31.size(); i++)
    {
        const PointI point = lastItPoints31.at(i);
        double m = OsmAnd::Utilities::distance31(point.x, point.y, toAdd->getStartPoint().x, toAdd->getStartPoint().y);
        if (m < minLastDist && minLastInd != i)
        {
            minLastInd = i;
            minLastDist = m;
        }
    }
    if (minLastDist > minStartDist)
    {
        if (minStartInd != toAdd->start->start)
        {
            std::shared_ptr<NetworkRouteSegment> newSeg = std::make_shared<NetworkRouteSegment>(toAdd->start->robj, toAdd->start->routeKey, minStartInd, toAdd->start->end);
            toAdd->setStart(newSeg);
        }
    }
    else
    {
        if (minLastInd != lastIt->end)
        {
            std::shared_ptr<NetworkRouteSegment> newSeg = std::make_shared<NetworkRouteSegment>(lastIt->robj, lastIt->routeKey, lastIt->start, minLastInd);
            it->setEnd(newSeg);
        }
    }
    it->addChain(toAdd);
    add(endChains, owner->rCtx->convertPointToLong(it->getEndPoint()), it);
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::NetworkRouteSelector_P::createGpxFile(const QList<std::shared_ptr<NetworkRouteSegmentChain>> &chains, const NetworkRouteKey &routeKey) const
{
    std::shared_ptr<GpxDocument> gpxFile = std::make_shared<GpxDocument>();
    std::shared_ptr<GpxDocument::Track> track = std::make_shared<GpxDocument::Track>();
    QList<int> sizes;
    QHash<int64_t, QVector<float>> heightArrayCache;
    for (const auto &c : constOf(chains)) {
        QList<std::shared_ptr<NetworkRouteSegment>> segmentList;
        segmentList.append(c->start);
        if (c->connected.size() > 0)
            segmentList.append(c->connected);

        std::shared_ptr<GpxDocument::TrkSegment> trkSegment = std::make_shared<GpxDocument::TrkSegment>();
        track->segments.append(trkSegment);
        int l = 0;
        std::shared_ptr<GpxDocument::WptPt> prev;
        for (auto &segment : segmentList)
        {
            QVector<float> heightArray;
            if (segment->robj)
            {
                auto it = heightArrayCache.find(segment->robj->id.id);
                if (it == heightArrayCache.end())
                {
                    it = heightArrayCache.insert(segment->robj->id.id, segment->robj->calculateHeightArray());
                }
                heightArray = *it;
            }
            int inc = segment->start < segment->end ? 1 : -1;
            for (int i = segment->start; ; i += inc)
            {
                std::shared_ptr<GpxDocument::WptPt> point = std::make_shared<GpxDocument::WptPt>();
                const PointI p = segment->robj->points31.at(i);
                point->position.latitude = OsmAnd::Utilities::get31LatitudeY(p.y);
                point->position.longitude = OsmAnd::Utilities::get31LongitudeX(p.x);
                if (heightArray.size() > i * 2 + 1)
                {
                    point->elevation = heightArray[i * 2 + 1];
                }
                trkSegment->points.append(point);
                if (prev)
                {
                    l += Utilities::distance(prev->position.longitude, prev->position.latitude, point->position.longitude, point->position.latitude);
                }
                prev = point;
                if (i == segment->end) {
                    break;
                }
            }
        }
        sizes.append(l);
    }
    QString log = QStringLiteral("");
    for (int s : sizes)
    {
        log += QString::number(s);
    }
    OsmAnd::LogPrintf(LogSeverityLevel::Debug, "Segments size %d: %s", track->segments.size(), qPrintable(log));
    gpxFile->tracks.append(track);
    gpxFile->networkRouteKeyTags = routeKey.tagsToGpx();
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
    ObfRoutingSectionReader::VisitorFunction visitor = [this, routeKey](const std::shared_ptr<const OsmAnd::Road>&road)
    {
        if (!routeKey)
        {
            return true;
        }
        QHash<QString, QString> tags = road->getResolvedAttributes();
        const auto routeKeys = NetworkRouteKey::getRouteKeys(tags);
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

std::shared_ptr<OsmAnd::NetworkRouteSegment> OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::getLast() const
{
    if (connected.size() > 0)
    {
        return connected.at(connected.size() - 1);
    }
    return start;
}

OsmAnd::PointI OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::getEndPoint() const
{
    const auto &seg = getLast();
    if (seg->robj &&  seg->robj->points31.size() > seg->end)
    {
        const PointI &p = seg->robj->points31.at(seg->end);
        return p;
    }
    PointI def(0, 0);
    return def;
}

OsmAnd::PointI OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::getStartPoint() const
{
    return start->robj->points31[start->start];
}

void OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::addChain(std::shared_ptr<NetworkRouteSegmentChain> &toAdd)
{
    connected.append(toAdd->start);
    connected.append(toAdd->connected);
}

void OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::setStart(std::shared_ptr<NetworkRouteSegment> &newStart)
{
    start = newStart;
}

void OsmAnd::NetworkRouteSelector_P::NetworkRouteSegmentChain::setEnd(std::shared_ptr<NetworkRouteSegment> &newEnd)
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
