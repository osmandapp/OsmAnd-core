#include "ImageMapLayerProvider.h"
#include "Logging.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkImage.h>
#include <SkBitmap.h>
#include "restore_internal_warnings.h"

#include "QtExtensions.h"
#include <QStandardPaths>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"
#include "IQueryController.h"
#include "SkiaUtilities.h"
#include "MapDataProviderHelpers.h"
#include "Utilities.h"

OsmAnd::ImageMapLayerProvider::ImageMapLayerProvider()
    : _priority(0)
    , _lastRequestedZoom(ZoomLevel0)
    , _threadPool(new QThreadPool())
    , _cacheThreadPool(new QThreadPool())
{
    const auto maxThreadCount = _threadPool->maxThreadCount();
    if (maxThreadCount > 4)
    {
        _threadPool->setMaxThreadCount(maxThreadCount - 2);
        _cacheThreadPool->setMaxThreadCount(2);
    }
    else if (maxThreadCount > 1)
    {
        _threadPool->setMaxThreadCount(maxThreadCount - 1);
        _cacheThreadPool->setMaxThreadCount(1);
    }
}

OsmAnd::ImageMapLayerProvider::~ImageMapLayerProvider()
{
    QMutexLocker scopedLocker(&_threadPoolMutex);
    _threadPool->clear();
    delete _threadPool;
    _cacheThreadPool->clear();
    delete _cacheThreadPool;
}

bool OsmAnd::ImageMapLayerProvider::supportsObtainImage() const
{
    return false;
}

sk_sp<const SkImage> OsmAnd::ImageMapLayerProvider::getEmptyImage() const
{
    SkBitmap bitmap;
    const auto tileSize = getTileSize();
    if (bitmap.tryAllocPixels(SkImageInfo::MakeN32Premul(tileSize, tileSize)))
    {
        bitmap.eraseColor(SK_ColorTRANSPARENT);
        return bitmap.asImage();
    }
    return nullptr;
}

sk_sp<const SkImage> OsmAnd::ImageMapLayerProvider::getImageWithData(
    const IMapDataProvider::Request& request,
    const QByteArray& imageData)
{
    const auto imageDimensions = obtainImageData(request, imageData);
    return imageDimensions > 0 ? SkiaUtilities::createSkImageARGB888With(imageData, imageDimensions) : nullptr;
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

    sk_sp<const SkImage> image = nullptr;
    QByteArray imageData = QByteArray();
    ZoomLevel zoom = request.zoom;
    TileId tileId = request.tileId;
    if (request.queryController != nullptr && request.queryController->isAborted())
    {
        return false;
    }
    else
    {
        const auto supportsImage = supportsObtainImage();

        // Obtain image
        image = supportsImage ? obtainImage(request) : getImageWithData(request, imageData);

        if (request.queryController != nullptr && request.queryController->isAborted())
            return false;

        const auto maxZoomShift = getMaxMissingDataZoomShift();
        const auto minZoom = getMinZoom();
        const auto zoomCount = request.zoom - minZoom > maxZoomShift ? maxZoomShift : request.zoom - minZoom;
        Request r;
        Request::copy(r, request);
        if (!image && request.cacheOnly)
        {
            // Search for overscale images in cache for the first requests (cache only)
            for (int zoomShift = 1; zoomShift <= zoomCount; zoomShift++)
            {
                zoom = static_cast<ZoomLevel>(request.zoom - zoomShift);
                tileId = Utilities::getTileIdOverscaledByZoomShift(request.tileId, zoomShift);
                r.zoom = zoom;
                r.tileId = tileId;
                image = supportsImage ? obtainImage(r) : getImageWithData(r, imageData);
                if (request.queryController != nullptr && request.queryController->isAborted())
                    return false;
                if (image)
                    break;
            }
        }
        if (!image)
        {
            const auto emptyImage = getEmptyImage();
            if (emptyImage && !request.cacheOnly)
            {
                // Check for overscale images to avoid rendering blank image instead
                r.cacheOnly = true;
                for (int zoomShift = 1; zoomShift <= zoomCount; zoomShift++)
                {
                    r.zoom = static_cast<ZoomLevel>(request.zoom - zoomShift);
                    r.tileId = Utilities::getTileIdOverscaledByZoomShift(request.tileId, zoomShift);
                    image = supportsImage ? obtainImage(r) : getImageWithData(r, imageData);
                    if (request.queryController != nullptr && request.queryController->isAborted())
                        return false;
                    if (image)
                        break;
                }
                if (!image)
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
            }
            else
            {
                outData.reset();
            }
            return true;
        }
    }

    if (!image)
        return false;

    if (!performAdditionalChecks(image))
        return false;

    // Return tile
    outData.reset(new IRasterMapLayerProvider::Data(
        tileId,
        zoom,
        getAlphaChannelPresence(),
        getTileDensityFactor(),
        image));
    return true;
}

void OsmAnd::ImageMapLayerProvider::obtainDataAsync(
    const IMapDataProvider::Request& request_,
    const IMapDataProvider::ObtainDataAsyncCallback callback,
    const bool collectMetric /*= false*/)
{
    const auto& request = MapDataProviderHelpers::castRequest<Request>(request_);
    setLastRequestedZoom(request.zoom);

    const auto selfWeak = std::weak_ptr<ImageMapLayerProvider>(shared_from_this());
    const auto requestClone = request_.clone();
    const QRunnableFunctor::Callback task =
    [selfWeak, requestClone, callback, collectMetric]
    (const QRunnableFunctor* const runnable)
    {
        const auto self = selfWeak.lock();
        if (self)
        {
            std::shared_ptr<IMapDataProvider::Data> data;
            std::shared_ptr<Metric> metric;
            const auto& r = MapDataProviderHelpers::castRequest<Request>(*requestClone);
            bool requestSucceeded = false;
            if (r.zoom == self->getLastRequestedZoom())
                requestSucceeded = self->obtainData(*requestClone, data, collectMetric ? &metric : nullptr);

            callback(self.get(), requestSucceeded, data, metric);
        }
    };

    const auto taskRunnable = new QRunnableFunctor(task);
    taskRunnable->setAutoDelete(true);
    int priority = getAndDecreasePriority();
    QMutexLocker scopedLocker(&_threadPoolMutex);
    if (request.cacheOnly)
    {
        if (_cacheThreadPool)
            _cacheThreadPool->start(taskRunnable, priority);
    }
    else
    {
        if (_threadPool)
            _threadPool->start(taskRunnable, priority);
    }
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

bool OsmAnd::ImageMapLayerProvider::performAdditionalChecks(const sk_sp<const SkImage>& image)
{
    return true;
}

OsmAnd::ImageMapLayerProvider::AsyncImageData::AsyncImageData(
    const ImageMapLayerProvider* const provider_,
    const ImageMapLayerProvider::Request& request_,
    const IMapDataProvider::ObtainDataAsyncCallback callback_)
    : provider(provider_)
    , request(std::dynamic_pointer_cast<const ImageMapLayerProvider::Request>(request_.clone()))
    , callback(callback_)
{
}

OsmAnd::ImageMapLayerProvider::AsyncImageData::~AsyncImageData()
{
}

void OsmAnd::ImageMapLayerProvider::AsyncImageData::submit(const bool requestSucceeded, const QByteArray& data) const
{
    std::shared_ptr<IMapDataProvider::Data> outData;

    if (requestSucceeded && !data.isNull())
    {
        const auto image = SkiaUtilities::createImageFromData(data);
        if (!image)
        {
            callback(provider, false, outData, nullptr);
            delete this;
            return;
        }

        outData.reset(new IRasterMapLayerProvider::Data(
            request->tileId,
            request->zoom,
            provider->getAlphaChannelPresence(),
            provider->getTileDensityFactor(),
            image));
    }

    callback(provider, requestSucceeded, outData, nullptr);
    delete this;
}
