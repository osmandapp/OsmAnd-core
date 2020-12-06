#ifndef _OSMAND_CORE_CORE_RESOURCES_AMENITY_ICON_PROVIDER_H_
#define _OSMAND_CORE_CORE_RESOURCES_AMENITY_ICON_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Map/IAmenityIconProvider.h>

namespace OsmAnd
{
    class OSMAND_CORE_API CoreResourcesAmenityIconProvider : public IAmenityIconProvider
    {
        Q_DISABLE_COPY_AND_MOVE(CoreResourcesAmenityIconProvider);

    private:
    protected:
    public:
        CoreResourcesAmenityIconProvider(
            const std::shared_ptr<const ICoreResourcesProvider>& coreResourcesProvider = getCoreResourcesProvider(),
            const float displayDensityFactor = 1.0f,
            const float symbolsScaleFactor = 1.0f);
        virtual ~CoreResourcesAmenityIconProvider();

        const std::shared_ptr<const ICoreResourcesProvider> coreResourcesProvider;
        const float displayDensityFactor;
        const float symbolsScaleFactor;

        virtual std::shared_ptr<SkBitmap> getIcon(
            const std::shared_ptr<const Amenity>& amenity,
            const ZoomLevel zoomLevel,
            const bool largeIcon = false) const Q_DECL_OVERRIDE;
        
        virtual TextRasterizer::Style getCaptionStyle(
            const std::shared_ptr<const Amenity>& amenity,
            const ZoomLevel zoomLevel) const Q_DECL_OVERRIDE;

        virtual QString getCaption(
            const std::shared_ptr<const Amenity>& amenity,
            const ZoomLevel zoomLevel) const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_CORE_RESOURCES_AMENITY_ICON_PROVIDER_H_)
