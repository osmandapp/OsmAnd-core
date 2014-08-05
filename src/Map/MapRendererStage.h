#ifndef _OSMAND_CORE_MAP_RENDERER_STAGE_H_
#define _OSMAND_CORE_MAP_RENDERER_STAGE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRendererResourcesManager.h"
#include "MapRendererConfiguration.h"
#include "MapRendererState.h"
#include "MapRendererInternalState.h"
#include "MapRendererSetupOptions.h"
#include "MapRendererDebugSettings.h"

namespace OsmAnd
{
    class MapRenderer;
    class GPUAPI;

    class MapRendererStage
    {
        Q_DISABLE_COPY(MapRendererStage);
    public:
        MapRendererStage(MapRenderer* const renderer);
        virtual ~MapRendererStage();

        MapRenderer* const renderer;
        const MapRendererResourcesManager& getResources();
        const std::unique_ptr<GPUAPI>& gpuAPI;
        const MapRendererState& currentState;
        const std::shared_ptr<const MapRendererConfiguration>& currentConfiguration;
        const MapRendererSetupOptions& setupOptions;
        const std::shared_ptr<const MapRendererDebugSettings>& debugSettings;

        virtual void initialize() = 0;
        virtual void render() = 0;
        virtual void release() = 0;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STAGE_H_)
