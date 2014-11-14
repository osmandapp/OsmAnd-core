#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_P.h"

#include "BinaryMapObjectsProvider.h"

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

bool OsmAnd::MapPrimitivesProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<MapPrimitivesProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new MapPrimitivesProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(
        tileId,
        zoom,
        tiledData,
        pOutMetric ? static_cast<MapPrimitivesProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr,
        queryController);
    outTiledData = tiledData;
    return result;
}

bool OsmAnd::MapPrimitivesProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<Data>& outTiledData,
    MapPrimitivesProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
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
