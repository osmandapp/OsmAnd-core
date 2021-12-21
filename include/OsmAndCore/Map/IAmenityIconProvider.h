#ifndef _OSMAND_CORE_I_AMENITY_ICON_PROVIDER_H_
#define _OSMAND_CORE_I_AMENITY_ICON_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <SkImage.h>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/MemoryCommon.h>
#include <OsmAndCore/TextRasterizer.h>

namespace OsmAnd
{
    class Amenity;

    class OSMAND_CORE_API IAmenityIconProvider
    {
        Q_DISABLE_COPY_AND_MOVE(IAmenityIconProvider);

    protected:
        IAmenityIconProvider();
    public:
        virtual ~IAmenityIconProvider();

        virtual sk_sp<SkImage> getIcon(
            const std::shared_ptr<const Amenity>& amenity,
            const ZoomLevel zoomLevel,
            const bool largeIcon = false) const = 0;
        
        virtual TextRasterizer::Style getCaptionStyle(
            const std::shared_ptr<const Amenity>& amenity,
            const ZoomLevel zoomLevel) const = 0;

        virtual QString getCaption(
            const std::shared_ptr<const Amenity>& amenity,
            const ZoomLevel zoomLevel) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_AMENITY_ICON_PROVIDER_H_)
