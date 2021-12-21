#include "MapRasterMetricsLayerProvider.h"
#include "MapRasterMetricsLayerProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::MapRasterMetricsLayerProvider::MapRasterMetricsLayerProvider(
    const std::shared_ptr<MapRasterLayerProvider>& rasterBitmapTileProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new MapRasterMetricsLayerProvider_P(this))
    , rasterBitmapTileProvider(rasterBitmapTileProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::MapRasterMetricsLayerProvider::~MapRasterMetricsLayerProvider()
{
}

OsmAnd::MapStubStyle OsmAnd::MapRasterMetricsLayerProvider::getDesiredStubsStyle() const
{
    return rasterBitmapTileProvider->getDesiredStubsStyle();
}

float OsmAnd::MapRasterMetricsLayerProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::MapRasterMetricsLayerProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::MapRasterMetricsLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::MapRasterMetricsLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::MapRasterMetricsLayerProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::MapRasterMetricsLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);
}

bool OsmAnd::MapRasterMetricsLayerProvider::obtainMetricsTile(
    const Request& request,
    std::shared_ptr<Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outData, pOutMetric);
}

OsmAnd::ZoomLevel OsmAnd::MapRasterMetricsLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapRasterMetricsLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::MapRasterMetricsLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelPresence alphaChannelPresence_,
    const float densityFactor_,
    const sk_sp<const SkImage>& image_,
    const std::shared_ptr<const MapRasterLayerProvider::Data>& rasterizedBinaryMap_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelPresence_, densityFactor_, image_, pRetainableCacheMetadata_)
    , rasterizedBinaryMap(rasterizedBinaryMap_)
{
}

OsmAnd::MapRasterMetricsLayerProvider::Data::~Data()
{
    release();
}
