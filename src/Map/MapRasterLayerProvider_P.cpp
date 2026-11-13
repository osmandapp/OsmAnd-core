#include "MapRasterLayerProvider_P.h"
#include "MapRasterLayerProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_Metrics.h"
#include "MapPrimitiviser.h"
#include "MapRasterizer.h"
#include "Stopwatch.h"
#include "MapRendererPerformanceMetrics.h"
#include "MapTileDiskCache.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkImage.h>
#include <SkPixmap.h>
#include <SkStream.h>
#include <include/encode/SkWebpEncoder.h>
#include "restore_internal_warnings.h"

namespace
{
    const float kWebpQuality = 80.0f;

    QByteArray encodeWebp(const sk_sp<SkImage>& image)
    {
        SkPixmap pixmap;
        if (!image->peekPixels(&pixmap))
            return QByteArray();
        SkDynamicMemoryWStream stream;
        SkWebpEncoder::Options options;
        options.fCompression = SkWebpEncoder::Compression::kLossy;
        options.fQuality = kWebpQuality;
        if (!SkWebpEncoder::Encode(&stream, pixmap, options))
            return QByteArray();
        const auto data = stream.detachAsData();
        return QByteArray(static_cast<const char*>(data->data()), static_cast<int>(data->size()));
    }

    sk_sp<SkImage> decodeWebp(const QByteArray& bytes)
    {
        const auto encoded = SkImage::MakeFromEncoded(SkData::MakeWithCopy(bytes.constData(), bytes.size()));
        // decode here, on the worker thread, not when the texture is uploaded
        return encoded ? encoded->makeRasterImage() : nullptr;
    }
}

OsmAnd::MapRasterLayerProvider_P::MapRasterLayerProvider_P(MapRasterLayerProvider* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapRasterLayerProvider_P::~MapRasterLayerProvider_P()
{
}

bool OsmAnd::MapRasterLayerProvider_P::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<MapRasterLayerProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new MapRasterLayerProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<MapRasterLayerProvider::Data> data;
    const auto result = obtainRasterizedTile(
        MapDataProviderHelpers::castRequest<MapRasterLayerProvider::Request>(request),
        data,
        pOutMetric ? static_cast<MapRasterLayerProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr);
    outData = data;

    return result;
}

bool OsmAnd::MapRasterLayerProvider_P::obtainRasterizedTile(
    const MapRasterLayerProvider::Request& request, 
    std::shared_ptr<MapRasterLayerProvider::Data>& outData,
    MapRasterLayerProvider_Metrics::Metric_obtainData* const metric)
{
    const auto& queryController = request.queryController;

    if (queryController->isAborted())
        return false;

    const Stopwatch totalStopwatch(metric != nullptr);

    // Rendered tile from the disk cache: no map objects or primitives needed
    uint64_t diskCacheEnvironment = 0;
    const auto diskCache = owner->primitivesProvider->getDiskCache(request.zoom, diskCacheEnvironment);
    MapTileDiskCache::Key diskCacheKey;
    if (diskCache)
    {
        diskCacheKey.environment = diskCacheEnvironment;
        diskCacheKey.kind = MapTileDiskCache::DataKind::RasterImage;
        diskCacheKey.tileId = request.tileId;
        diskCacheKey.zoom = request.zoom;
        // line widths follow the detailed zoom (tilted view), background is a provider option
        diskCacheKey.variant = (request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom)
            | (owner->fillBackground ? 0x100 : 0);
        const auto bytes = diskCache->get(diskCacheKey);
        const auto image = bytes.isEmpty() ? nullptr : decodeWebp(bytes);
        if (image)
        {
            if (OsmAnd::isPerformanceMetricsEnabled())
                OsmAnd::getPerformanceMetrics().rasterStart(request.tileId);
            outData.reset(new MapRasterLayerProvider::Data(
                request.tileId,
                request.zoom,
                AlphaChannelPresence::NotPresent,
                owner->getTileDensityFactor(),
                image,
                nullptr,
                nullptr));
            if (metric)
                metric->elapsedTime += totalStopwatch.elapsed();
            if (OsmAnd::isPerformanceMetricsEnabled())
                OsmAnd::getPerformanceMetrics().rasterFinish(request.tileId, request.zoom);
            return true;
        }
    }

    // Obtain offline map primitives tile
    std::shared_ptr<MapPrimitivesProvider::Data> primitivesTile;
    owner->primitivesProvider->obtainTiledPrimitives(
        request,
        primitivesTile,
        metric ? metric->findOrAddSubmetricOfType<MapPrimitivesProvider_Metrics::Metric_obtainData>().get() : nullptr);

    if (queryController->isAborted())
        return false;

    if (!primitivesTile || primitivesTile->primitivisedObjects->isEmpty())
    {
        outData.reset();

        if (metric)
            metric->elapsedTime += totalStopwatch.elapsed();

        return true;
    }

    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().rasterStart(request.tileId);

    // Perform actual rasterization
    const auto rasterizationStart = MapTileDiskCache::getThreadTimeMs();
    const auto image = rasterize(request, primitivesTile, metric);
    const auto generationTimeMs = primitivesTile->generationTimeMs
        + static_cast<float>(MapTileDiskCache::getThreadTimeMs() - rasterizationStart);

    if (queryController->isAborted())
        return false;

    if (!image)
    {
        if (metric)
            metric->elapsedTime += totalStopwatch.elapsed();

        return false;
    }

    // a tile drawn with the detail of another zoom is shown only while the zoom changes: it is rendered again
    // with its own detail once the zoom settles, so it is not worth keeping
    // encoded to WebP on the cache's thread, so a tile rendered for the first time does not wait for it
    if (diskCache && (request.detailedZoom == InvalidZoomLevel || request.detailedZoom == request.zoom))
    {
        diskCache->putLater(diskCacheKey,
            [image]
            () -> QByteArray
            {
                return encodeWebp(image);
            },
            qRound(generationTimeMs));
    }

    // Or supply newly rasterized tile
    outData.reset(new MapRasterLayerProvider::Data(
        request.tileId,
        request.zoom,
        AlphaChannelPresence::NotPresent,
        owner->getTileDensityFactor(),
        image,
        primitivesTile,
        new RetainableCacheMetadata(primitivesTile->retainableCacheMetadata)));

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().rasterFinish(request.tileId, request.zoom);

    return true;
}

void OsmAnd::MapRasterLayerProvider_P::initialize()
{
    _mapRasterizer.reset(new MapRasterizer(owner->primitivesProvider->primitiviser->environment));
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider_P::getMinZoom() const
{
    return owner->primitivesProvider->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::MapRasterLayerProvider_P::getMaxZoom() const
{
    return owner->primitivesProvider->getMaxZoom();
}

OsmAnd::MapRasterLayerProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapPrimitivesRetainableCacheMetadata_)
    : binaryMapPrimitivesRetainableCacheMetadata(binaryMapPrimitivesRetainableCacheMetadata_)
{
}

OsmAnd::MapRasterLayerProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
}
