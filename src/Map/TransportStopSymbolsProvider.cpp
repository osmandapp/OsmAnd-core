#include "TransportStopSymbolsProvider.h"
#include "TransportStopSymbolsProvider_P.h"

#include "MapDataProviderHelpers.h"
#include "TransportStop.h"

OsmAnd::TransportStopSymbolsProvider::TransportStopSymbolsProvider(
    const std::shared_ptr<const IObfsCollection>& obfsCollection_,
    const int symbolsOrder_,
    const std::shared_ptr<const TransportRoute>& transportRoute_ /*= nullptr*/,
    const std::shared_ptr<const ITransportRouteIconProvider>& transportRouteIconProvider_ /*= nullptr*/)
    : _p(new TransportStopSymbolsProvider_P(this))
    , obfsCollection(obfsCollection_)
    , symbolsOrder(symbolsOrder_)
    , transportRoute(transportRoute_)
    , transportRouteIconProvider(transportRouteIconProvider_)
{
}

OsmAnd::TransportStopSymbolsProvider::~TransportStopSymbolsProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::TransportStopSymbolsProvider::getMinZoom() const
{
    return MinZoomLevel;
}

OsmAnd::ZoomLevel OsmAnd::TransportStopSymbolsProvider::getMaxZoom() const
{
    return MaxZoomLevel;
}

bool OsmAnd::TransportStopSymbolsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::TransportStopSymbolsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::TransportStopSymbolsProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::TransportStopSymbolsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}

OsmAnd::TransportStopSymbolsProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const QList< std::shared_ptr<MapSymbolsGroup> >& symbolsGroups_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledSymbolsProvider::Data(tileId_, zoom_, symbolsGroups_, pRetainableCacheMetadata_)
{
}

OsmAnd::TransportStopSymbolsProvider::Data::~Data()
{
    release();
}

OsmAnd::TransportStopSymbolsProvider::TransportStopSymbolsGroup::TransportStopSymbolsGroup(
    const std::shared_ptr<const TransportStop>& transportStop_)
    : transportStop(transportStop_)
{
}

OsmAnd::TransportStopSymbolsProvider::TransportStopSymbolsGroup::~TransportStopSymbolsGroup()
{
}

bool OsmAnd::TransportStopSymbolsProvider::TransportStopSymbolsGroup::obtainSharingKey(SharingKey& outKey) const
{
    return false;
}

bool OsmAnd::TransportStopSymbolsProvider::TransportStopSymbolsGroup::obtainSortingKey(SortingKey& outKey) const
{
    outKey = static_cast<SharingKey>(transportStop->id);
    return true;
}

QString OsmAnd::TransportStopSymbolsProvider::TransportStopSymbolsGroup::toString() const
{
    return {};
}
