#ifndef _OSMAND_CORE_NETWORK_ROUTE_SELECTOR_H_
#define _OSMAND_CORE_NETWORK_ROUTE_SELECTOR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/ObfRoutingSectionReader.h>
#include <OsmAndCore/NetworkRouteContext.h>
#include <OsmAndCore/GpxDocument.h>

namespace OsmAnd
{
    class IObfsCollection;
    class Road;

    class NetworkRouteSelector_P;
    class OSMAND_CORE_API NetworkRouteSelector
    {
        Q_DISABLE_COPY_AND_MOVE(NetworkRouteSelector);
    private:
        PrivateImplementation<NetworkRouteSelector_P> _p;
    protected:
    public:
        NetworkRouteSelector(
            const std::shared_ptr<const IObfsCollection>& obfsCollection,
            const std::shared_ptr<ObfRoutingSectionReader::DataBlocksCache>& cache = nullptr);
        virtual ~NetworkRouteSelector();

        const std::shared_ptr<NetworkRouteContext> rCtx;
        NetworkRouteSelectorFilter filter;
        
        QList<std::shared_ptr<const Road>> getRoutes(
            const AreaI area31,
            NetworkRouteKey * routeKey = nullptr,
            const RoutingDataLevel dataLevel = RoutingDataLevel::Detailed,
            QList<std::shared_ptr<const ObfRoutingSectionReader::DataBlock>> * const outReferencedCacheEntries = nullptr) const;
        
        QMap<NetworkRouteKey, std::shared_ptr<GpxDocument>> getRoutes(const AreaI area31, bool loadRoutes, NetworkRouteKey * routeKey = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_NETWORK_ROUTE_SELECTOR_H_)
