#ifndef _OSMAND_CORE_MAP_RENDERER_TYPES_H_
#define _OSMAND_CORE_MAP_RENDERER_TYPES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    enum class RasterMapLayerId : int32_t
    {
        Invalid = -1,

        BaseLayer,
        Overlay0,
        Overlay1,
        Overlay2,
        Overlay3,

        __LAST,
    };
    enum {
        RasterMapLayersCount = static_cast<unsigned>(RasterMapLayerId::__LAST)
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TYPES_H_)
