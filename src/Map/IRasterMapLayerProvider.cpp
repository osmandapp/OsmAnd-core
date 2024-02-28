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
    , images(QHash<int64_t, sk_sp<const SkImage>>())
{
    images.insert(0, qMove(image_));
}

OsmAnd::IRasterMapLayerProvider::Data::Data(
    TileId tileId_,
    ZoomLevel zoom_,
    AlphaChannelPresence alphaChannelPresence_,
    float densityFactor_,
    QHash<int64_t, sk_sp<const SkImage>>& images_,
    const RetainableCacheMetadata* pRetainableCacheMetadata_ /*= nullptr*/)
    : IMapLayerProvider::Data(tileId_, zoom_, pRetainableCacheMetadata_)
    , alphaChannelPresence(alphaChannelPresence_)
    , densityFactor(densityFactor_)
    , images(qMove(images_))
{
}

OsmAnd::IRasterMapLayerProvider::Data::~Data()
{
    for (auto& image : images)
        image.reset();
    release();
}
