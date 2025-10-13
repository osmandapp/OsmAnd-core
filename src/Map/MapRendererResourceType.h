#ifndef _OSMAND_CORE_MAP_RENDERER_RESOURCE_TYPE_H_
#define _OSMAND_CORE_MAP_RENDERER_RESOURCE_TYPE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"

namespace OsmAnd
{
    enum class MapRendererResourceType
    {
        Unknown = -1,

        ElevationData,
        MapLayer,
        Symbols,
        Map3DObjects,

        __LAST
    };
    enum
    {
        MapRendererResourceTypesCount = static_cast<int>(MapRendererResourceType::__LAST)
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_BASE_RESOURCE_H_)
