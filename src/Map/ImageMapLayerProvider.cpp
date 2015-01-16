#include "ImageMapLayerProvider.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkImageDecoder.h>
#include <SkStream.h>
#include "restore_internal_warnings.h"

OsmAnd::ImageMapLayerProvider::ImageMapLayerProvider()
{
}

OsmAnd::ImageMapLayerProvider::~ImageMapLayerProvider()
{
}

bool OsmAnd::ImageMapLayerProvider::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<IMapTiledDataProvider::Data>& outTiledData,
    std::shared_ptr<Metric>* pOutMetric /*= nullptr*/,
    const IQueryController* const queryController /*= nullptr*/)
{
    // Obtain image data
    const auto image = obtainImage(tileId, zoom);
    if (image.isNull())
    {
        outTiledData.reset();
        return true;
    }

    // Decode image data
    std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    SkMemoryStream imageStream(image.constData(), image.length(), false);
    if (!SkImageDecoder::DecodeStream(&imageStream, bitmap.get(), SkColorType::kUnknown_SkColorType, SkImageDecoder::kDecodePixels_Mode))
        return false;
    
    // Return tile
    outTiledData.reset(new IRasterMapLayerProvider::Data(
        tileId,
        zoom,
        getAlphaChannelPresence(),
        getTileDensityFactor(),
        bitmap));
    return true;
}
