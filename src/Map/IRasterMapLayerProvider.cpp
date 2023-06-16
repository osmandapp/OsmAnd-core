#include "IRasterMapLayerProvider.h"

#include "MapDataProviderHelpers.h"

OsmAnd::IRasterMapLayerProvider::IRasterMapLayerProvider()
{
}

OsmAnd::IRasterMapLayerProvider::~IRasterMapLayerProvider()
{
}

bool OsmAnd::IRasterMapLayerProvider::obtainRasterTile(
    const Request& request,
    std::shared_ptr<Data>& outRasterTile,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    return MapDataProviderHelpers::obtainData(this, request, outRasterTile, pOutMetric);
}

OsmAnd::IRasterMapLayerProvider::Data::Data(
    TileId tileId_,
    ZoomLevel zoom_,
    AlphaChannelPresence alphaChannelPresence_,
    float densityFactor_,
    sk_sp<const SkImage> image_,
    const RetainableCacheMetadata* pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapLayerProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , alphaChannelPresence(alphaChannelPresence_)
    , densityFactor(densityFactor_)
    , image(qMove(image_))
{
}

OsmAnd::IRasterMapLayerProvider::Data::~Data()
{
    if (image)
        image.reset();
    release();
}
