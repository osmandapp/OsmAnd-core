#include "BinaryMapRasterBitmapTileProvider.h"
#include "BinaryMapRasterBitmapTileProvider_P.h"

#include "BinaryMapPrimitivesProvider.h"
#include "Primitiviser.h"
#include "MapPresentationEnvironment.h"

OsmAnd::BinaryMapRasterBitmapTileProvider::BinaryMapRasterBitmapTileProvider(
    BinaryMapRasterBitmapTileProvider_P* const p_,
    const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_)
    : _p(p_)
    , primitivesProvider(primitivesProvider_)
{
    _p->initialize();
}

OsmAnd::BinaryMapRasterBitmapTileProvider::~BinaryMapRasterBitmapTileProvider()
{
}

float OsmAnd::BinaryMapRasterBitmapTileProvider::getTileDensityFactor() const
{
    return primitivesProvider->primitiviser->environment->displayDensityFactor;
}

uint32_t OsmAnd::BinaryMapRasterBitmapTileProvider::getTileSize() const
{
    return primitivesProvider->tileSize;
}

bool OsmAnd::BinaryMapRasterBitmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

bool OsmAnd::BinaryMapRasterBitmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData* const metric,
    const IQueryController* const queryController)
{
    return _p->obtainData(tileId, zoom, outTiledData, metric, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapRasterBitmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapRasterizedTile::BinaryMapRasterizedTile(
    const std::shared_ptr<const BinaryMapPrimitivesTile>& binaryMapTile_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : RasterBitmapTile(bitmap_, alphaChannelData_, densityFactor_, tileId_, zoom_)
    , binaryMapTile(binaryMapTile_)
{
}

OsmAnd::BinaryMapRasterizedTile::~BinaryMapRasterizedTile()
{
}
