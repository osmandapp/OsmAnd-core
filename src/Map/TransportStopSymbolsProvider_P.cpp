#include "TransportStopSymbolsProvider_P.h"
#include "TransportStopSymbolsProvider.h"

#include "ICoreResourcesProvider.h"
#include "ObfDataInterface.h"
#include "MapDataProviderHelpers.h"
#include "BillboardRasterMapSymbol.h"
#include "SkiaUtilities.h"
#include "Utilities.h"
#include "TransportStop.h"
#include "TransportRoute.h"
#include "TransportStopsInAreaSearch.h"

OsmAnd::TransportStopSymbolsProvider_P::TransportStopSymbolsProvider_P(TransportStopSymbolsProvider* owner_)
    : owner(owner_)
{
}

OsmAnd::TransportStopSymbolsProvider_P::~TransportStopSymbolsProvider_P()
{
}

bool OsmAnd::TransportStopSymbolsProvider_P::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    const auto& request = MapDataProviderHelpers::castRequest<TransportStopSymbolsProvider::Request>(request_);

    if (pOutMetric)
        pOutMetric->reset();

    if (request.zoom > owner->getMaxZoom() || request.zoom < owner->getMinZoom() || request.zoom < 12)
    {
        outData.reset();
        return true;
    }

    const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);

    const auto bbox31 = AreaI(tileBBox31.top() >> (31 - TransportStopsInAreaSearch::TRANSPORT_STOP_ZOOM),
                              tileBBox31.left() >> (31 - TransportStopsInAreaSearch::TRANSPORT_STOP_ZOOM),
                              tileBBox31.bottom() >> (31 - TransportStopsInAreaSearch::TRANSPORT_STOP_ZOOM),
                              tileBBox31.right() >> (31 - TransportStopsInAreaSearch::TRANSPORT_STOP_ZOOM));

    const auto dataInterface = owner->obfsCollection->obtainDataInterface(
        &bbox31,
        request.zoom,
        request.zoom,
        ObfDataTypesMask().set(ObfDataType::Transport));

    QList< std::shared_ptr<MapSymbolsGroup> > mapSymbolsGroups;
    const auto requestedZoom = request.zoom;
    
    const ObfTransportSectionReader::TransportStopVisitorFunction visitorFunction =
    [this, requestedZoom, &mapSymbolsGroups, bbox31]
    (const std::shared_ptr<const OsmAnd::TransportStop>& transportStop) -> bool
    {
        if (!owner->transportRouteIconProvider)
            return false;
        
        const auto position31 = Utilities::convertLatLonTo31(transportStop->location);
        if (bbox31.contains(position31.x >> (31 - TransportStopsInAreaSearch::TRANSPORT_STOP_ZOOM), position31.y >> (31 - TransportStopsInAreaSearch::TRANSPORT_STOP_ZOOM)))
        {
            const auto icon = owner->transportRouteIconProvider->getIcon(owner->transportRoute, requestedZoom, false);
            if (!icon)
                return false;
            
            const auto mapSymbolsGroup = std::make_shared<TransportStopSymbolsGroup>(transportStop);
            
            const auto mapSymbol = std::make_shared<BillboardRasterMapSymbol>(mapSymbolsGroup);
            mapSymbol->order = owner->symbolsOrder;
            mapSymbol->image = icon;
            mapSymbol->size = PointI(icon->width(), icon->height());
            mapSymbol->languageId = LanguageId::Invariant;
            mapSymbol->position31 = position31;
            mapSymbolsGroup->symbols.push_back(mapSymbol);
            
            mapSymbolsGroups.push_back(mapSymbolsGroup);
            
            return true;
        }
        else
        {
            return false;
        }
    };
    
    if (owner->transportRoute != nullptr)
    {
        const auto stops = owner->transportRoute->forwardStops;
        for (auto stop : stops)
            visitorFunction(stop);
    }
    else
    {
        auto stringTable = std::make_shared<ObfSectionInfo::StringTable>();
        dataInterface->searchTransportIndex(
                                            nullptr,
                                            &bbox31,
                                            stringTable.get(),
                                            visitorFunction);
    }
    
    outData.reset(new TransportStopSymbolsProvider::Data(
        request.tileId,
        request.zoom,
        mapSymbolsGroups));
    
    return true;
}

OsmAnd::TransportStopSymbolsProvider_P::RetainableCacheMetadata::RetainableCacheMetadata()
{
}

OsmAnd::TransportStopSymbolsProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
