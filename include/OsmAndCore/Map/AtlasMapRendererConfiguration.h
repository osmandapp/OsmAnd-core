#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_CONFIGURATION_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_CONFIGURATION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapRendererConfiguration.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API AtlasMapRendererConfiguration : public MapRendererConfiguration
    {
        AtlasMapRendererConfiguration();
        virtual ~AtlasMapRendererConfiguration();

        unsigned int referenceTileSizeOnScreenInPixels;

        virtual void copyTo(MapRendererConfiguration& other) const;
        virtual std::shared_ptr<MapRendererConfiguration> createCopy() const;

        SWIG_DECLARE_UPCAST_POINTER(AtlasMapRendererConfiguration, MapRendererConfiguration);
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_CONFIGURATION_H_)
