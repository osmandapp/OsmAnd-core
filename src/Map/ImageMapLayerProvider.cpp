#include "ImageMapLayerProvider.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImageDecoder.h>
#include <SkStream.h>
#include "restore_internal_warnings.h"

#include "MapDataProviderHelpers.h"

OsmAnd::ImageMapLayerProvider::ImageMapLayerProvider()
{
}

OsmAnd::ImageMapLayerProvider::~ImageMapLayerProvider()
{
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
        outData.reset();
        return true;
    }

    // Decode image data
    std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    SkMemoryStream imageStream(image.constData(), image.length(), false);
    if (!SkImageDecoder::DecodeStream(&imageStream, bitmap.get(), SkColorType::kUnknown_SkColorType, SkImageDecoder::kDecodePixels_Mode))
        return false;

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

    if (!supportsNaturalObtainDataAsync())
        return MapDataProviderHelpers::nonNaturalObtainDataAsync(this, request, callback, collectMetric);

    obtainImageAsync(request, new AsyncImage(this, request, callback));
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
