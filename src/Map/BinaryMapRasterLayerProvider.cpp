#include "BinaryMapRasterLayerProvider.h"
#include "BinaryMapRasterLayerProvider_P.h"

#include "BinaryMapPrimitivesProvider.h"
#include "Primitiviser.h"
#include "MapPresentationEnvironment.h"

OsmAnd::BinaryMapRasterLayerProvider::BinaryMapRasterLayerProvider(
    BinaryMapRasterLayerProvider_P* const p_,
    const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_)
    : _p(p_)
    , primitivesProvider(primitivesProvider_)
{
    _p->initialize();
}

OsmAnd::BinaryMapRasterLayerProvider::~BinaryMapRasterLayerProvider()
{
}

float OsmAnd::BinaryMapRasterLayerProvider::getTileDensityFactor() const
{
    return primitivesProvider->primitiviser->environment->displayDensityFactor;
}

uint32_t OsmAnd::BinaryMapRasterLayerProvider::getTileSize() const
{
    return primitivesProvider->tileSize;
}

bool OsmAnd::BinaryMapRasterLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    std::shared_ptr<Data> tiledData;
    const auto result = _p->obtainData(tileId, zoom, tiledData, nullptr, queryController);
    outTiledData = tiledData;
    return result;
}

bool OsmAnd::BinaryMapRasterLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<Data>& outTiledData,
    BinaryMapRasterLayerProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterLayerProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterLayerProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapRasterLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& binaryMapData_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IRasterMapLayerProvider::Data(tileId_, zoom_, alphaChannelData_, densityFactor_, bitmap_, pRetainableCacheMetadata_)
    , binaryMapData(binaryMapData_)
{
}

OsmAnd::BinaryMapRasterLayerProvider::Data::~Data()
{
    release();
}
