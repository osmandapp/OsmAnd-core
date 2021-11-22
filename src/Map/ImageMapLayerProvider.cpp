#include "ImageMapLayerProvider.h"
#include "Logging.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkData.h>
#include <SkImage.h>
#include "restore_internal_warnings.h"

#include "QtExtensions.h"
#include <QStandardPaths>

#include "OsmAndCore.h"
#include "MapDataProviderHelpers.h"
#include "QRunnableFunctor.h"
#include "IQueryController.h"

#include "MapDataProviderHelpers.h"

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

bool OsmAnd::ImageMapLayerProvider::supportsObtainImageBitmap() const
{
    return false;
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

const std::shared_ptr<const SkBitmap> OsmAnd::ImageMapLayerProvider::decodeBitmap(const QByteArray& data)
{
    const auto skData = SkData::MakeWithoutCopy(data.constData(), data.length());
    if (!skData)
    {
        return nullptr;
    }
    const auto skImage = SkImage::MakeFromEncoded(skData);
    if (!skImage)
    {
        LogPrintf(LogSeverityLevel::Error,
            "Failed to decode image tile");
        return nullptr;
    }

    // TODO: replace SkBitmap with SkImage
    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    if (skImage->asLegacyBitmap(bitmap.get()))
    {
        return bitmap;
    }
    return nullptr;
}

const std::shared_ptr<const SkBitmap> OsmAnd::ImageMapLayerProvider::obtainImageBitmap(const OsmAnd::IMapTiledDataProvider::Request& request)
{
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
    
    std::shared_ptr<const SkBitmap> bitmap;
    if (supportsObtainImageBitmap())
    {
        bitmap = obtainImageBitmap(request);

        if (request.queryController != nullptr && request.queryController->isAborted())
            return false;

        if (!bitmap)
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
    }
    else
    {
        // Obtain image data
        const auto image = obtainImage(request);
        
        if (request.queryController != nullptr && request.queryController->isAborted())
            return false;

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
        bitmap = decodeBitmap(image);
    }
    
    if (!bitmap)
        return false;

    performAdditionalChecks(bitmap);

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

void OsmAnd::ImageMapLayerProvider::performAdditionalChecks(std::shared_ptr<const SkBitmap> bitmap)
{
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

void OsmAnd::ImageMapLayerProvider::AsyncImage::submit(const bool requestSucceeded, const QByteArray& data) const
{
    std::shared_ptr<IMapDataProvider::Data> outData;

    if (requestSucceeded && !data.isNull())
    {
        const auto skData = SkData::MakeWithoutCopy(data.constData(), data.length());
        if (!skData)
        {
            callback(provider, false, outData, nullptr);
            delete this;
            return;
        }
        const auto skImage = SkImage::MakeFromEncoded(skData);
        if (!skImage)
        {
            callback(provider, false, outData, nullptr);
            delete this;
            return;
        }

        // TODO: replace SkBitmap with SkImage
        const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
        if (skImage->asLegacyBitmap(bitmap.get()))
        {
            outData.reset(new IRasterMapLayerProvider::Data(
                request->tileId,
                request->zoom,
                provider->getAlphaChannelPresence(),
                provider->getTileDensityFactor(),
                bitmap));
        }
	else
	{
            callback(provider, false, outData, nullptr);
            delete this;
            return;
	}
    }

    callback(provider, requestSucceeded, outData, nullptr);
    delete this;
}
