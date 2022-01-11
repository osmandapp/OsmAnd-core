#include "WeatherRasterLayerProvider.h"
#include "WeatherRasterLayerProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::WeatherRasterLayerProvider::WeatherRasterLayerProvider(
    const QString& geotiffPath_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/,
    const QString& cacheDbFilename_ /*= QString::null*/)
    : _p(new WeatherRasterLayerProvider_P(this))
    , geotiffPath(geotiffPath_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
    , cacheDbFilename(cacheDbFilename_)
{
}

OsmAnd::WeatherRasterLayerProvider::~WeatherRasterLayerProvider()
{
}

OsmAnd::MapStubStyle OsmAnd::WeatherRasterLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

float OsmAnd::WeatherRasterLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::WeatherRasterLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::WeatherRasterLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::WeatherRasterLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::WeatherRasterLayerProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::WeatherRasterLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::WeatherRasterLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}
