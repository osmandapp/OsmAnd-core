#include "MapRasterLayerProvider_P.h"
#include "MapRasterLayerProvider.h"

#include "MapDataProviderHelpers.h"
#include "MapPrimitivesProvider.h"
#include "MapPrimitivesProvider_Metrics.h"
#include "MapPrimitiviser.h"
#include "MapRasterizer.h"
#include "Stopwatch.h"
#include "Logging.h"
#include "MapRendererPerformanceMetrics.h"

#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <chrono>
#include <deque>
#include <thread>
#include <SkBitmap.h>
#include <SkPixmap.h>
#if defined(__ANDROID__)
#include <dlfcn.h>
#include <android/bitmap.h>
#endif

namespace OsmAnd
{
    bool zbFlagL();
    quint64 zbTileImageKey(const MapPrimitivesProvider* owner, const TileId tileId, const ZoomLevel zoom);
    QByteArray zbTileImageRead(const quint64 key);
    void zbTileImageWrite(const quint64 key, const QByteArray& encoded, qint64* outTotal, int* outCount);
}

namespace
{
    // Android system WebP codec, resolved at runtime (API 30+)
    struct ZbCodec
    {
        typedef bool (*WriteFn)(void* ctx, const void* data, size_t size);
        typedef int (*CompressFn)(const AndroidBitmapInfo* info, int32_t dataspace, const void* pixels,
            int32_t format, int32_t quality, void* ctx, WriteFn fn);
        typedef int (*CreateFn)(const void* buffer, size_t length, void** outDecoder);
        typedef int (*SetFormatFn)(void* decoder, int32_t format);
        typedef const void* (*HeaderFn)(const void* decoder);
        typedef int32_t (*DimFn)(const void* header);
        typedef size_t (*StrideFn)(void* decoder);
        typedef int (*DecodeFn)(void* decoder, void* pixels, size_t stride, size_t size);
        typedef void (*DeleteFn)(void* decoder);
        bool ok = false;
        CompressFn compress = nullptr;
        CreateFn create = nullptr;
        SetFormatFn setFormat = nullptr;
        HeaderFn header = nullptr;
        DimFn width = nullptr, height = nullptr;
        StrideFn stride = nullptr;
        DecodeFn decode = nullptr;
        DeleteFn destroy = nullptr;
    };
    const ZbCodec& zbCodec()
    {
        static const ZbCodec codec = []() -> ZbCodec
        {
            ZbCodec c;
#if defined(__ANDROID__)
            void* lib = dlopen("libjnigraphics.so", RTLD_NOW);
            if (!lib)
                return c;
            c.compress = reinterpret_cast<ZbCodec::CompressFn>(dlsym(lib, "AndroidBitmap_compress"));
            c.create = reinterpret_cast<ZbCodec::CreateFn>(dlsym(lib, "AImageDecoder_createFromBuffer"));
            c.setFormat = reinterpret_cast<ZbCodec::SetFormatFn>(dlsym(lib, "AImageDecoder_setAndroidBitmapFormat"));
            c.header = reinterpret_cast<ZbCodec::HeaderFn>(dlsym(lib, "AImageDecoder_getHeaderInfo"));
            c.width = reinterpret_cast<ZbCodec::DimFn>(dlsym(lib, "AImageDecoderHeaderInfo_getWidth"));
            c.height = reinterpret_cast<ZbCodec::DimFn>(dlsym(lib, "AImageDecoderHeaderInfo_getHeight"));
            c.stride = reinterpret_cast<ZbCodec::StrideFn>(dlsym(lib, "AImageDecoder_getMinimumStride"));
            c.decode = reinterpret_cast<ZbCodec::DecodeFn>(dlsym(lib, "AImageDecoder_decodeImage"));
            c.destroy = reinterpret_cast<ZbCodec::DeleteFn>(dlsym(lib, "AImageDecoder_delete"));
            c.ok = c.compress && c.create && c.setFormat && c.header && c.width && c.height && c.stride && c.decode && c.destroy;
#endif
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, ">>>> 0 TILECODEC 0.0: webp %d", c.ok ? 1 : 0);
            return c;
        }();
        return codec;
    }

    bool zbEncodeWrite(void* ctx, const void* data, size_t size)
    {
        static_cast<QByteArray*>(ctx)->append(static_cast<const char*>(data), static_cast<int>(size));
        return true;
    }

    QByteArray zbEncode(const sk_sp<SkImage>& image)
    {
        QByteArray out;
#if defined(__ANDROID__)
        SkPixmap pm;
        if (!image->peekPixels(&pm) || pm.colorType() != kRGBA_8888_SkColorType)
            return out;
        AndroidBitmapInfo info;
        memset(&info, 0, sizeof(info));
        info.width = pm.width();
        info.height = pm.height();
        info.stride = static_cast<uint32_t>(pm.rowBytes());
        info.format = ANDROID_BITMAP_FORMAT_RGBA_8888;
        info.flags = pm.alphaType() == kOpaque_SkAlphaType ? 1 /* OPAQUE */ : 0 /* PREMUL */;
        const int32_t ADATASPACE_SRGB_ = 142671872;
        const int32_t FORMAT_WEBP_LOSSY = 3;
        if (zbCodec().compress(&info, ADATASPACE_SRGB_, pm.addr(), FORMAT_WEBP_LOSSY, 80, &out, &zbEncodeWrite) != 0)
            out.clear();
#endif
        return out;
    }

    sk_sp<SkImage> zbDecode(const QByteArray& data)
    {
        const auto& c = zbCodec();
        void* decoder = nullptr;
        if (c.create(data.constData(), static_cast<size_t>(data.size()), &decoder) != 0 || !decoder)
            return nullptr;
        sk_sp<SkImage> result;
        c.setFormat(decoder, 1 /* ANDROID_BITMAP_FORMAT_RGBA_8888 */);
        const auto header = c.header(decoder);
        const int w = c.width(header), h = c.height(header);
        SkBitmap bitmap;
        if (bitmap.tryAllocPixels(SkImageInfo::Make(w, h, kRGBA_8888_SkColorType, kPremul_SkAlphaType)))
        {
            if (c.decode(decoder, bitmap.getPixels(), bitmap.rowBytes(), bitmap.computeByteSize()) == 0)
            {
                bitmap.setImmutable();
                result = bitmap.asImage();
            }
        }
        c.destroy(decoder);
        return result;
    }

    // one background encoder so the first (cold) pass does not pay for WebP
    struct ZbEncodeQueue
    {
        QMutex mutex;
        QWaitCondition cond;
        std::deque< std::pair<quint64, sk_sp<SkImage>> > items;
        bool started = false;
    };
    ZbEncodeQueue& zbQueue()
    {
        static ZbEncodeQueue q;
        return q;
    }
    void zbEnqueue(const quint64 key, const sk_sp<SkImage>& image, const int zoom)
    {
        auto& q = zbQueue();
        QMutexLocker locker(&q.mutex);
        if (q.items.size() >= 64)
            return;
        q.items.emplace_back(key, image);
        if (!q.started)
        {
            q.started = true;
            std::thread([]()
            {
                auto& q = zbQueue();
                for (;;)
                {
                    std::pair<quint64, sk_sp<SkImage>> item;
                    {
                        QMutexLocker locker(&q.mutex);
                        while (q.items.empty())
                            q.cond.wait(&q.mutex);
                        item = q.items.front();
                        q.items.pop_front();
                    }
                    const auto t0 = std::chrono::steady_clock::now();
                    const auto bytes = zbEncode(item.second);
                    if (bytes.isEmpty())
                        continue;
                    qint64 total = 0;
                    int count = 0;
                    OsmAnd::zbTileImageWrite(item.first, bytes, &total, &count);
                    const auto ms = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, ">>>> 0 TILEPACKW %f: %d bytes %dx%d (pack %lld bytes %d tiles)",
                        ms, bytes.size(), item.second->width(), item.second->height(), static_cast<long long>(total), count);
                }
            }).detach();
        }
        q.cond.wakeOne();
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

    // zoom-bench L: rasterized tile from the disk cache, no primitives needed
    const bool zbUseL = zbFlagL() && request.zoom <= ZoomLevel15 && zbCodec().ok;
    quint64 zbKeyL = 0;
    if (zbUseL)
    {
        const auto t0 = std::chrono::steady_clock::now();
        zbKeyL = zbTileImageKey(owner->primitivesProvider.get(), request.tileId, request.zoom) * 31 + (owner->fillBackground ? 1 : 0);
        const auto bytes = zbTileImageRead(zbKeyL);
        if (!bytes.isEmpty())
        {
            if (OsmAnd::isPerformanceMetricsEnabled())
                OsmAnd::getPerformanceMetrics().rasterStart(request.tileId);
            const auto image = zbDecode(bytes);
            if (image)
            {
                outData.reset(new MapRasterLayerProvider::Data(
                    request.tileId,
                    request.zoom,
                    AlphaChannelPresence::NotPresent,
                    owner->getTileDensityFactor(),
                    image,
                    nullptr,
                    nullptr));
                const auto ms = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                LogPrintf(LogSeverityLevel::Info, ">>>> %lld TILECACHE %f: hit %d bytes %dx%d@%d",
                    static_cast<long long>(now), ms, bytes.size(), request.tileId.x, request.tileId.y, static_cast<int>(request.zoom));
                if (metric)
                    metric->elapsedTime += totalStopwatch.elapsed();
                if (OsmAnd::isPerformanceMetricsEnabled())
                    OsmAnd::getPerformanceMetrics().rasterFinish(request.tileId, request.zoom);
                return true;
            }
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
    const auto image = rasterize(request, primitivesTile, metric);

    if (queryController->isAborted())
        return false;

    if (!image)
    {
        if (metric)
            metric->elapsedTime += totalStopwatch.elapsed();

        return false;
    }

    if (zbUseL)
        zbEnqueue(zbKeyL, image, static_cast<int>(request.zoom));

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
