#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_P.h"

#include "MapDataProviderHelpers.h"

OsmAnd::MapPrimitivesProvider::MapPrimitivesProvider(
    const std::shared_ptr<IMapObjectsProvider>& mapObjectsProvider_,
    const std::shared_ptr<MapPrimitiviser>& primitiviser_,
    const unsigned int tileSize_ /*= 256*/,
    const Mode mode_ /*= Mode::WithSurface*/)
    : _p(new MapPrimitivesProvider_P(this))
    , mapObjectsProvider(mapObjectsProvider_)
    , primitiviser(primitiviser_)
    , tileSize(tileSize_)
    , mode(mode_)
{
}

OsmAnd::MapPrimitivesProvider::~MapPrimitivesProvider()
{
}

OsmAnd::ZoomLevel OsmAnd::MapPrimitivesProvider::getMinZoom() const
{
    return mapObjectsProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapPrimitivesProvider::getMaxZoom() const
{
    return mapObjectsProvider->getMaxZoom();
}

bool OsmAnd::MapPrimitivesProvider::obtainTiledPrimitives(
    const Request& request,
    std::shared_ptr<Data>& outTiledPrimitives,
    MapPrimitivesProvider_Metrics::Metric_obtainData* metric /*= nullptr*/)
{
    return _p->obtainTiledPrimitives(request, outTiledPrimitives, metric);
}

bool OsmAnd::MapPrimitivesProvider::supportsNaturalObtainData() const
{
    return true;
}

bool OsmAnd::MapPrimitivesProvider::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return _p->obtainData(request, outData, pOutMetric);
}

bool OsmAnd::MapPrimitivesProvider::supportsNaturalObtainDataAsync() const
{
    return false;
}

void OsmAnd::MapPrimitivesProvider::obtainDataAsync(
    const IMapDataProvider::Request& request,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    MapDataProviderHelpers::nonNaturalObtainDataAsync(shared_from_this(), request, callback, collectMetric);
}

OsmAnd::MapPrimitivesProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const std::shared_ptr<const IMapObjectsProvider::Data>& mapObjectsData_,
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapTiledDataProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , mapObjectsData(mapObjectsData_)
    , primitivisedObjects(primitivisedObjects_)
{
}

OsmAnd::MapPrimitivesProvider::Data::~Data()
{
    release();
}
