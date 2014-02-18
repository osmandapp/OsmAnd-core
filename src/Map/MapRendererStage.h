#ifndef _OSMAND_CORE_MAP_RENDERER_STAGE_H_
#define _OSMAND_CORE_MAP_RENDERER_STAGE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRendererResources.h"
#include "MapRendererConfiguration.h"
#include "MapRendererState.h"
#include "MapRendererInternalState.h"

namespace OsmAnd
{
    class MapRenderer;
    class GPUAPI;

    class MapRendererStage
    {
        Q_DISABLE_COPY(MapRendererStage);
    public:
        // Declare short aliases for resource-related entities
        typedef OsmAnd::MapRendererResources Resources;
        typedef OsmAnd::MapRendererResources::ResourceType ResourceType;
        typedef OsmAnd::MapRendererResources::ResourceState ResourceState;
        typedef OsmAnd::MapRendererInternalState InternalState;
    public:
        MapRendererStage(MapRenderer* const renderer);
        virtual ~MapRendererStage();

        MapRenderer* const renderer;
        const MapRendererResources& getResources();
        const std::unique_ptr<GPUAPI>& gpuAPI;
        const MapRendererState& currentState;
        const MapRendererConfiguration& currentConfiguration;

        virtual void initialize() = 0;
        virtual void render() = 0;
        virtual void release() = 0;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STAGE_H_)
