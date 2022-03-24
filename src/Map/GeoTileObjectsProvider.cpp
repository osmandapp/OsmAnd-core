#include "GeoTileObjectsProvider.h"
#include "GeoTileObjectsProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::GeoTileObjectsProvider::GeoTileObjectsProvider(
    const std::shared_ptr<WeatherTileResourcesManager> resourcesManager,
    const QDateTime& dateTime_,
    const BandIndex band_,
    const uint32_t cacheSize_ /*= 0*/)
    : _p(new GeoTileObjectsProvider_P(this))
    , _resourcesManager(resourcesManager)
    , dateTime(dateTime_)
    , band(band_)
    , cacheSize(cacheSize_)
{
}

OsmAnd::GeoTileObjectsProvider::~GeoTileObjectsProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::GeoTileObjectsProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::GeoTileObjectsProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

bool OsmAnd::GeoTileObjectsProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::GeoTileObjectsProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::GeoTileObjectsProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::GeoTileObjectsProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}
