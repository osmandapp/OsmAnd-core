#include "MapRasterLayerProvider_Software_P.h"
#include "MapRasterLayerProvider_Software.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkCanvas.h>
#include "restore_internal_warnings.h"

#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_P.h"
#include "MapRasterLayerCache.h"
#include "MapRasterizer.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::MapRasterLayerProvider_Software_P::MapRasterLayerProvider_Software_P(MapRasterLayerProvider_Software* owner_)
    : MapRasterLayerProvider_P(owner_)
    , owner(owner_)
{
}

OsmAnd::MapRasterLayerProvider_Software_P::~MapRasterLayerProvider_Software_P()
{
}

std::shared_ptr<OsmAnd::MapRasterLayerProvider::Data> OsmAnd::MapRasterLayerProvider_Software_P::readCachedRaster(
    const MapRasterLayerProvider::Request& request)
{
    const auto cache = owner->getLocalCache();
    if (!cache)
        return nullptr;

    const auto primitives = owner->primitivesProvider->_p.shared_ptr();
    const auto key = cache->key(request);
    if (key.isEmpty() || cache != primitives->getRasterCache())
        return nullptr;

    sk_sp<SkImage> image;
    if (!cache->readImage(request, key, image))
        return nullptr;
    if (request.queryController->isAborted() || cache != owner->getLocalCache()
        || cache != primitives->getRasterCache())
        return nullptr;

    return std::make_shared<MapRasterLayerProvider::Data>(
        request.tileId, request.zoom, AlphaChannelPresence::NotPresent, owner->getTileDensityFactor(), image, nullptr,
        new MapPrimitivesProvider_P::RasterHitMetadata(primitives, cache, request));
}

sk_sp<SkImage> OsmAnd::MapRasterLayerProvider_Software_P::rasterize(
    const MapRasterLayerProvider::Request& request,
    const std::shared_ptr<const MapPrimitivesProvider::Data>& primitivesTile,
    MapRasterLayerProvider_Metrics::Metric_obtainData* const metric)
{
    // Allocate rasterization target
    const auto tileSize = owner->getTileSize();
    SkBitmap bitmap;
    if (!bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(tileSize, tileSize)))
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to allocate buffer for rasterization surface %dx%d",
            tileSize,
            tileSize);
        return nullptr;
    }

    // Create rasterization canvas
    SkCanvas canvas(bitmap);

    // Perform actual rasterization
    if (!owner->fillBackground)
        canvas.clear(SK_ColorTRANSPARENT);
    _mapRasterizer->rasterize(
        Utilities::tileBoundingBox31(request.tileId, request.zoom),
        primitivesTile->primitivisedObjects,
        canvas,
        owner->fillBackground,
        nullptr,
        metric ? metric->findOrAddSubmetricOfType<MapRasterizer_Metrics::Metric_rasterize>().get() : nullptr,
        request.queryController);

    bitmap.setImmutable();
    const auto image = bitmap.asImage();
    const auto cache = primitivesTile->rasterCache.lock();
    if (cache && !primitivesTile->rasterCacheKey.isEmpty() && !request.queryController->isAborted()
        && cache == owner->getLocalCache() && cache == owner->primitivesProvider->_p->getRasterCache()
        && cache->key(request) == primitivesTile->rasterCacheKey)
        cache->write(request, primitivesTile->rasterCacheKey, image, primitivesTile);
    return image;
}
