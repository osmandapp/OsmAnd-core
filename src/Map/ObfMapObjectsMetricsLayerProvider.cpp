#include "ObfMapObjectsMetricsLayerProvider.h"
#include "ObfMapObjectsMetricsLayerProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::ObfMapObjectsMetricsLayerProvider::ObfMapObjectsMetricsLayerProvider(
    const std::shared_ptr<ObfMapObjectsProvider>& dataProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new ObfMapObjectsMetricsLayerProvider_P(this))
    , dataProvider(dataProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::ObfMapObjectsMetricsLayerProvider::~ObfMapObjectsMetricsLayerProvider()
{
}

OsmAnd::MapStubStyle OsmAnd::ObfMapObjectsMetricsLayerProvider::getDesiredStubsStyle() const
{
    return MapStubStyle::Unspecified;
}

float OsmAnd::ObfMapObjectsMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::ObfMapObjectsMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::ObfMapObjectsMetricsLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::ObfMapObjectsMetricsLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::ObfMapObjectsMetricsLayerProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::ObfMapObjectsMetricsLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

bool OsmAnd::ObfMapObjectsMetricsLayerProvider::obtainMetricsTile(
    const Request& request,
    std::shared_ptr<Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outData, pOutMetric);
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::ObfMapObjectsMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::ObfMapObjectsMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelPresence alphaChannelPresence_,
    const float densityFactor_,
    const sk_sp<const SkImage>& image_,
    const std::shared_ptr<const ObfMapObjectsProvider::Data>& binaryMapData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelPresence_, densityFactor_, image_, pRetainableCacheMetadata_)
    , binaryMapData(binaryMapData_)
{
}

OsmAnd::ObfMapObjectsMetricsLayerProvider::Data::~Data()
{
    release();
}
