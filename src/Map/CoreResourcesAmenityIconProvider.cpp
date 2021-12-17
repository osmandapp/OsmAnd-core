#include "CoreResourcesAmenityIconProvider.h"

#include "ICoreResourcesProvider.h"
#include "Amenity.h"
#include "SkiaUtilities.h"

OsmAnd::CoreResourcesAmenityIconProvider::CoreResourcesAmenityIconProvider(
    const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider_ /*= getCoreResourcesProvider()*/,
    const float displayDensityFactor_ /*= 1.0f*/,
    const float symbolsScaleFactor_ /*= 1.0f*/)
    : coreResourcesProvider(coreResourcesProvider_)
    , displayDensityFactor(displayDensityFactor_)
    , symbolsScaleFactor(symbolsScaleFactor_)
{
}

OsmAnd::CoreResourcesAmenityIconProvider::~CoreResourcesAmenityIconProvider()
{
}

sk_sp<SkImage> OsmAnd::CoreResourcesAmenityIconProvider::getIcon(
    const std::shared_ptr<const Amenity>& amenity,
    const ZoomLevel zoomLevel,
    const bool largeIcon /*= false*/) const
{
    const auto& decodedCategories = amenity->getDecodedCategories();

    const auto& iconPath = largeIcon
        ? QLatin1String("map/largeIcons/")
        : QLatin1String("map/icons/");
    const QLatin1String iconExtension(".png");

    for (const auto& decodedCategory : constOf(decodedCategories))
    {
        auto icon = coreResourcesProvider->getResourceAsImage(
            iconPath + decodedCategory.subcategory + iconExtension,
            displayDensityFactor);
        if (!icon)
        {
            icon = coreResourcesProvider->getResourceAsImage(
                iconPath + "" + iconExtension, //TODO: resolve poi_type in category by it's subcat and get tag/name
                displayDensityFactor);
        }
        if (!icon)
            continue;

        return SkiaUtilities::scaleImage(icon, symbolsScaleFactor, symbolsScaleFactor);
    }

    return nullptr;
}

OsmAnd::TextRasterizer::Style OsmAnd::CoreResourcesAmenityIconProvider::getCaptionStyle(
    const std::shared_ptr<const Amenity>& amenity,
    const ZoomLevel zoomLevel) const
{
    return TextRasterizer::Style();
}

QString OsmAnd::CoreResourcesAmenityIconProvider::getCaption(
    const std::shared_ptr<const Amenity>& amenity,
    const ZoomLevel zoomLevel) const
{
    return {};
}
