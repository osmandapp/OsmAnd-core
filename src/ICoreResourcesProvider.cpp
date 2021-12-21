#include "ICoreResourcesProvider.h"

#include "ignore_warnings_on_external_includes.h"
#include <SkBitmap.h>
#include <SkData.h>
#include <SkImage.h>
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

    const auto skData = SkData::MakeWithoutCopy(data.constData(), data.length());
    if (!skData)
    {
        return nullptr;
    }
    const auto skImage = SkImage::MakeFromEncoded(skData);
    if (!skImage)
    {
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

std::shared_ptr<SkBitmap> OsmAnd::ICoreResourcesProvider::getResourceAsBitmap(const QString& name) const
{
    bool ok = false;
    const auto data = getResource(name, &ok);
    if (!ok)
        return nullptr;
    
    const auto skData = SkData::MakeWithoutCopy(data.constData(), data.length());
    if (!skData)
    {
        return nullptr;
    }
    const auto skImage = SkImage::MakeFromEncoded(skData);
    if (!skImage)
    {
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
