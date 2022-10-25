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
#include "MapRenderer.h"

OsmAnd::ImageMapLayerProvider::ImageMapLayerProvider()
    : _priority(0)
    , _lastRequestedZoom(ZoomLevel0)
    , _threadPool(new QThreadPool())
{
}

OsmAnd::ImageMapLayerProvider::~ImageMapLayerProvider()
{
    _threadPool->clear();
    delete _threadPool;
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

    if (request.queryController != nullptr && request.queryController->isAborted())
        return false;

    sk_sp<const SkImage> image;
    if (supportsObtainImage())
    {
        image = obtainImage(request);

        if (request.queryController != nullptr && request.queryController->isAborted())
            return false;

        if (!image)
        {
            outData.reset();
            return true;
        }
    }
    else
    {
        // Obtain image data
        const auto imageData = obtainImageData(request);

        if (request.queryController != nullptr && request.queryController->isAborted())
            return false;

        if (imageData.isNull())
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
        image = SkiaUtilities::createImageFromData(imageData);
    }

    if (!image)
        return false;

    if (!performAdditionalChecks(image))
        return false;

    // Return tile
    outData.reset(new IRasterMapLayerProvider::Data(
        request.tileId,
        request.zoom,
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
            Request r;
            Request::copy(r, *requestClone);
            bool requestSucceeded = false;
            const auto maxZoomShift = r.zoom - MinZoomLevel > MapRenderer::MaxMissingDataZoomShift ?
                MapRenderer::MaxMissingDataZoomShift : r.zoom - MinZoomLevel;
            for (r.zoomShift = 0; r.zoomShift <= maxZoomShift; r.zoomShift++)
            {
                requestSucceeded = self->obtainData(r, data, collectMetric ? &metric : nullptr);
                if (!requestSucceeded || data)
                    break;                    
            }
            if (requestSucceeded && !data)
            {
                const auto emptyImage = self->getEmptyImage();
                if (emptyImage)
                {
                    // Return empty tile
                    data.reset(new IRasterMapLayerProvider::Data(
                        r.tileId,
                        r.zoom,
                        self->getAlphaChannelPresence(),
                        self->getTileDensityFactor(),
                        emptyImage));
                }
                else
                    data.reset();
            }
            callback(self.get(), requestSucceeded, data, metric);
        }
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
