#include "MapRasterLayerProvider.h"
#include "MapRasterLayerProvider_P.h"

#include "MapDataProviderHelpers.h"
#include "MapPrimitivesProvider.h"
#include "MapPrimitiviser.h"
#include "MapPresentationEnvironment.h"

OsmAnd::MapRasterLayerProvider::MapRasterLayerProvider(
    MapRasterLayerProvider_P* const p_,
    const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider_,
    const bool fillBackground_)
    : _p(p_)
    , primitivesProvider(primitivesProvider_)
    , fillBackground(fillBackground_)
{
    _p->initialize();
}

OsmAnd::MapRasterLayerProvider::~MapRasterLayerProvider()
{
}

OsmAnd::MapStubStyle OsmAnd::MapRasterLayerProvider::getDesiredStubsStyle() const
{
    return primitivesProvider->primitiviser->environment->getDesiredStubsStyle();
}

float OsmAnd::MapRasterLayerProvider::getTileDensityFactor() const
{
    return primitivesProvider->primitiviser->environment->displayDensityFactor;
}

uint32_t OsmAnd::MapRasterLayerProvider::getTileSize() const
{
    return primitivesProvider->tileSize;
}

bool OsmAnd::MapRasterLayerProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::MapRasterLayerProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::MapRasterLayerProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::MapRasterLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

bool OsmAnd::MapRasterLayerProvider::obtainRasterizedTile(
    const Request& request,
    std::shared_ptr<Data>& outData,
    MapRasterLayerProvider_Metrics::Metric_obtainData* const metric /*= nullptr*/)
{
    return _p->obtainRasterizedTile(request, outData, metric);
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::MapRasterLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelPresence alphaChannelPresence_,
    const float densityFactor_,
    const sk_sp<const SkImage>& image_,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& binaryMapData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelPresence_, densityFactor_, image_, pRetainableCacheMetadata_)
    , binaryMapData(binaryMapData_)
{
}

OsmAnd::MapRasterLayerProvider::Data::~Data()
{
    release();
}
