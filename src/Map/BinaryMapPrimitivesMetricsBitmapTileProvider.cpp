#include "BinaryMapPrimitivesMetricsBitmapTileProvider.h"
#include "BinaryMapPrimitivesMetricsBitmapTileProvider_P.h"

OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider::BinaryMapPrimitivesMetricsBitmapTileProvider(
    const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider_,
    const uint32_t tileSize_ /*= 256*/,
    const float densityFactor_ /*= 1.0f*/)
    : _p(new BinaryMapPrimitivesMetricsBitmapTileProvider_P(this))
    , primitivesProvider(primitivesProvider_)
    , tileSize(tileSize_)
    , densityFactor(densityFactor_)
{
}

OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider::~BinaryMapPrimitivesMetricsBitmapTileProvider()
{
}

float OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider::getTileDensityFactor() const
{
    return densityFactor;
}

uint32_t OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider::getTileSize() const
{
    return tileSize;
}

bool OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    const IQueryController* const queryController /*= nullptr*/)
{
    return _p->obtainData(tileId, zoom, outTiledData, queryController);
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::BinaryMapPrimitivesMetricsBitmapTileProvider::getMaxZoom() const
{
    return _p->getMaxZoom();
}

OsmAnd::BinaryMapPrimitivesMetricsTile::BinaryMapPrimitivesMetricsTile(
    const std::shared_ptr<const BinaryMapPrimitivesTile>& binaryMapPrimitivesTile_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const float densityFactor_,
    const TileId tileId_,
    const ZoomLevel zoom_)
    : RasterBitmapTile(bitmap_, AlphaChannelData::NotPresent, densityFactor_, tileId_, zoom_)
    , binaryMapPrimitivesTile(binaryMapPrimitivesTile_)
{
}

OsmAnd::BinaryMapPrimitivesMetricsTile::~BinaryMapPrimitivesMetricsTile()
{
}
