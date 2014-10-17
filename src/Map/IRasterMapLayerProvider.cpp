#include "IRasterMapLayerProvider.h"

OsmAnd::IRasterMapLayerProvider::IRasterMapLayerProvider()
{
}

OsmAnd::IRasterMapLayerProvider::~IRasterMapLayerProvider()
{
}

OsmAnd::IRasterMapLayerProvider::Data::Data(
    const TileId tileId_,
    const ZoomLevel zoom_,
    const AlphaChannelData alphaChannelData_,
    const float densityFactor_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const RetainableCacheMetadata* const pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapLayerProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , alphaChannelData(alphaChannelData_)
    , densityFactor(densityFactor_)
    , bitmap(bitmap_)
{
}

OsmAnd::IRasterMapLayerProvider::Data::~Data()
{
    release();
}
