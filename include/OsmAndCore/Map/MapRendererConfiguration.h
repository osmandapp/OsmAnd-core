#ifndef _OSMAND_CORE_MAP_RENDERER_CONFIGURATION_H_
#define _OSMAND_CORE_MAP_RENDERER_CONFIGURATION_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    enum class TextureFilteringQuality
    {
        Normal,
        Good,
        Best
    };

    struct OSMAND_CORE_API MapRendererConfiguration
    {
        MapRendererConfiguration();
        ~MapRendererConfiguration();

        TextureFilteringQuality texturesFilteringQuality;
        bool limitTextureColorDepthBy16bits;
        uint32_t heixelsPerTileSide;
        bool paletteTexturesAllowed;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_CONFIGURATION_H_)
