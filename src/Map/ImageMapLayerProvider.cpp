#include "ImageMapLayerProvider.h"
#include "Logging.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImageDecoder.h>
#include <SkStream.h>
#include "restore_internal_warnings.h"

#include "QtExtensions.h"
#include <QStandardPaths>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"

#include "MapDataProviderHelpers.h"

OsmAnd::ImageMapLayerProvider::ImageMapLayerProvider() : emptyImage(nullptr),
_priority(0),
_lastRequestedZoom(ZoomLevel0),
_threadPool(new QThreadPool())
{
}

OsmAnd::ImageMapLayerProvider::~ImageMapLayerProvider()
{
    waitForTasksDone();
    delete _threadPool;
}

void OsmAnd::ImageMapLayerProvider::waitForTasksDone(bool clear /* = true*/)
{
    _threadPool->clear();
    _threadPool->waitForDone();
}

const std::shared_ptr<const SkBitmap> OsmAnd::ImageMapLayerProvider::getEmptyImage()
{
    SkBitmap bitmap;
    // Create a bitmap that will be hold entire symbol (if target is empty)
    const auto tileSize = getTileSize();
    if (bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(tileSize, tileSize)))
    {
        bitmap.eraseColor(SK_ColorTRANSPARENT);
        return std::make_shared<SkBitmap>(bitmap);
    }
    return nullptr;
}

bool OsmAnd::ImageMapLayerProvider::obtainData(
    const IMapDataProvider::Request& request_,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric /*= nullptr*/)
{
    const auto& request = MapDataProviderHelpers::castRequest<Request>(request_);
    if (pOutMetric)
        pOutMetric->reset();

    // Check provider can supply this zoom level
    if (request.zoom > getMaxZoom() || request.zoom < getMinZoom())
    {
        outData.reset();
        return true;
    }

    if (!supportsNaturalObtainData())
        return MapDataProviderHelpers::nonNaturalObtainData(this, request, outData, pOutMetric);

    // Obtain image data
    const auto image = obtainImage(request);
    if (image.isNull())
    {
        const auto emptyImage = getEmptyImage();
        if (emptyImage)
        {
            // Return empty tile
            outData.reset(new IRasterMapLayerProvider::Data(
                request.tileId,
                request.zoom,
                getAlphaChannelPresence(),
                getTileDensityFactor(),
                emptyImage));
        }
        else
        {
            outData.reset();
        }
        return true;
    }
    
    // Decode image data
    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    if (!SkImageDecoder::DecodeMemory(
            image.constData(), image.size(),
            bitmap.get(),
            SkColorType::kUnknown_SkColorType,
            SkImageDecoder::kDecodePixels_Mode))
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to decode image tile");

        return false;
    }
    
    // Return tile
    outData.reset(new IRasterMapLayerProvider::Data(
        request.tileId,
        request.zoom,
        getAlphaChannelPresence(),
        getTileDensityFactor(),
        bitmap));
    return true;
}

void OsmAnd::ImageMapLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request_,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& request = MapDataProviderHelpers::castRequest<Request>(request_);
    setLastRequestedZoom(request.zoom);
    
    const auto requestClone = request_.clone();
    const QRunnableFunctor::Callback task =
    [this, requestClone, callback, collectMetric]
    (const QRunnableFunctor* const runnable)
    {
        std::shared_ptr<IMapDataProvider::Data> data;
        std::shared_ptr<Metric> metric;
        const auto& r = MapDataProviderHelpers::castRequest<Request>(*requestClone);
        
        bool requestSucceeded = false;
        if (r.zoom == getLastRequestedZoom())
            requestSucceeded = this->obtainData(*requestClone, data, collectMetric ? &metric : nullptr);

        callback(this, requestSucceeded, data, metric);
    };
    
    const auto taskRunnable = new QRunnableFunctor(task);
    taskRunnable->setAutoDelete(true);
    int priority = getAndDecreasePriority();
    _threadPool->start(taskRunnable, priority);
}

OsmAnd::ZoomLevel OsmAnd::ImageMapLayerProvider::getLastRequestedZoom() const
{
    QReadLocker scopedLocker(&_lock);
    
    return _lastRequestedZoom;
}

void OsmAnd::ImageMapLayerProvider::setLastRequestedZoom(const ZoomLevel zoomLevel)
{
    QWriteLocker scopedLocker(&_lock);

    if (_lastRequestedZoom != zoomLevel)
        _priority = 0;
    
    _lastRequestedZoom = zoomLevel;
}

int OsmAnd::ImageMapLayerProvider::getAndDecreasePriority()
{
    QWriteLocker scopedLocker(&_lock);
    
    _priority--;
    
    return _priority;
}

OsmAnd::ImageMapLayerProvider::AsyncImage::AsyncImage(
    const ImageMapLayerProvider* const provider_,
    const ImageMapLayerProvider::Request& request_,
    const IMapDataProvider::ObtainDataAsyncCallback callback_)
    : provider(provider_)
    , request(std::dynamic_pointer_cast<const ImageMapLayerProvider::Request>(request_.clone()))
    , callback(callback_)
{
}

OsmAnd::ImageMapLayerProvider::AsyncImage::~AsyncImage()
{
}

void OsmAnd::ImageMapLayerProvider::AsyncImage::submit(const bool requestSucceeded, const QByteArray& image) const
{
    std::shared_ptr<IMapDataProvider::Data> data;

    if (requestSucceeded && !image.isNull())
    {
        // Decode image data
        std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
        SkMemoryStream imageStream(image.constData(), image.length(), false);
        if (SkImageDecoder::DecodeStream(&imageStream, bitmap.get(), SkColorType::kUnknown_SkColorType, SkImageDecoder::kDecodePixels_Mode))
        {
            data.reset(new IRasterMapLayerProvider::Data(
                request->tileId,
                request->zoom,
                provider->getAlphaChannelPresence(),
                provider->getTileDensityFactor(),
                bitmap));
        }
        else
        {
            callback(provider, false, data, nullptr);
            delete this;
            return;
        }
    }

    callback(provider, requestSucceeded, data, nullptr);
    delete this;
}
