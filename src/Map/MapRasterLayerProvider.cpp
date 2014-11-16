#include "MapRasterLayerProvider.h"
#include "MapRasterLayerProvider_P.h"

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

float OsmAnd::MapRasterLayerProvider::getTileDensityFactor() const
{
    return primitivesProvider->primitiviser->environment->displayDensityFactor;
}

uint32_t OsmAnd::MapRasterLayerProvider::getTileSize() const
{
    return primitivesProvider->tileSize;
}

bool OsmAnd::MapRasterLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<MapRasterLayerProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new MapRasterLayerProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(
        tileId,
        zoom,
        tiledData,
        pOutMetric ? static_cast<MapRasterLayerProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr,
        queryController);
    outTiledData = tiledData;

    return result;
}

bool OsmAnd::MapRasterLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<Data>& outTiledData,
    MapRasterLayerProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
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
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& binaryMapData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelData_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , binaryMapData(binaryMapData_)
{
}

OsmAnd::MapRasterLayerProvider::Data::~Data()
{
    release();
}
