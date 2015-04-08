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
