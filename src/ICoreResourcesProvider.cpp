#include "ICoreResourcesProvider.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkImageDecoder.h>
#include "restore_internal_warnings.h"

OsmAnd::ICoreResourcesProvider::ICoreResourcesProvider()
{
}

OsmAnd::ICoreResourcesProvider::~ICoreResourcesProvider()
{
}

std::shared_ptr<SkBitmap> OsmAnd::ICoreResourcesProvider::getResourceAsBitmap(
    const QString& name,
    const float displayDensityFactor) const
{
    bool ok = false;
    const auto data = getResource(name, displayDensityFactor, &ok);
    if (!ok)
        return nullptr;

    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    SkMemoryStream dataStream(data.constData(), data.length(), false);
    if (!SkImageDecoder::DecodeStream(&dataStream, bitmap.get(), SkColorType::kUnknown_SkColorType, SkImageDecoder::kDecodePixels_Mode))
        return nullptr;

    return bitmap;
}

std::shared_ptr<SkBitmap> OsmAnd::ICoreResourcesProvider::getResourceAsBitmap(const QString& name) const
{
    bool ok = false;
    const auto data = getResource(name, &ok);
    if (!ok)
        return nullptr;

    const std::shared_ptr<SkBitmap> bitmap(new SkBitmap());
    SkMemoryStream dataStream(data.constData(), data.length(), false);
    if (!SkImageDecoder::DecodeStream(&dataStream, bitmap.get(), SkColorType::kUnknown_SkColorType, SkImageDecoder::kDecodePixels_Mode))
        return nullptr;

    return bitmap;
}
