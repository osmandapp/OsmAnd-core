#ifndef _OSMAND_CORE_MAP_RENDERER_TYPES_PRIVATE_H_
#define _OSMAND_CORE_MAP_RENDERER_TYPES_PRIVATE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"

namespace OsmAnd
{
    class MapSymbolsGroup;
    class MapSymbol;
    class MapRendererBaseResource;

    struct PublishOrUnpublishMapSymbol
    {
        std::shared_ptr<const MapSymbolsGroup> symbolGroup;
        std::shared_ptr<const MapSymbol> symbol;
        std::shared_ptr<MapRendererBaseResource> resource;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_TYPES_PRIVATE_H_)
