#include "NetworkRouteSelector.h"
#include "NetworkRouteSelector_P.h"

OsmAnd::NetworkRouteSelector::NetworkRouteSelector(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache_, /*= nullptr*/
    const std::shared_ptr<const IQueryController>& queryController_)
    : _p(new NetworkRouteSelector_P(this))
    , queryController(queryController_)
{
    rCtx = std::make_shared<NetworkRouteContext>(obfsCollection_, cache_);
}

OsmAnd::NetworkRouteSelector::~NetworkRouteSelector()
{
}

QList<std::shared_ptr<const OsmAnd::Road>> OsmAnd::NetworkRouteSelector::getRoutes(const AreaI area31,
                                                                                   NetworkRouteKey * routeKey,
                                                                                   const RoutingDataLevel dataLevel,
                                                                                   QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>> * const outReferencedCacheEntries) const
{
    return _p->getRoutes(area31, routeKey, dataLevel, outReferencedCacheEntries);
}

QHash<OsmAnd::NetworkRouteKey, std::shared_ptr<OsmAnd::GpxDocument>> OsmAnd::NetworkRouteSelector::getRoutes(const AreaI area31, bool loadRoutes, NetworkRouteKey * routeKey) const
{
    return _p->getRoutes(area31, loadRoutes, routeKey);
}

void OsmAnd::NetworkRouteSelector::setNetworkRouteKeyFilter(NetworkRouteKey & routeKey)
{
    rCtx->setNetworkRouteKeyFilter(routeKey);
}
