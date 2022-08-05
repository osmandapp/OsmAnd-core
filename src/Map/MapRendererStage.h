#ifndef _OSMAND_CORE_MAP_RENDERER_STAGE_H_
#define _OSMAND_CORE_MAP_RENDERER_STAGE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRenderer.h"
#include "MapRendererResourcesManager.h"
#include "MapRendererConfiguration.h"
#include "MapRendererState.h"
#include "MapRendererInternalState.h"
#include "MapRendererSetupOptions.h"
#include "MapRendererDebugSettings.h"

namespace OsmAnd
{
    class GPUAPI;

    class MapRendererStage
    {
        Q_DISABLE_COPY_AND_MOVE(MapRendererStage);
    protected:
        void invalidateFrame();
    public:
        MapRendererStage(MapRenderer* const renderer);
        virtual ~MapRendererStage();

        MapRenderer* const renderer;

        const MapRendererResourcesManager& getResources() const;

        const std::unique_ptr<GPUAPI>& gpuAPI;
        const MapRendererSetupOptions& setupOptions;
        const std::shared_ptr<const MapRendererConfiguration>& currentConfiguration;
        const MapRendererState& currentState;
        const MapRendererInternalState& internalState;
        const std::shared_ptr<const MapRendererDebugSettings>& debugSettings;
        QReadWriteLock& publishedMapSymbolsByOrderLock;
        const MapRenderer::PublishedMapSymbolsByOrder& publishedMapSymbolsByOrder;

        virtual bool initialize() = 0;
        virtual bool render(IMapRenderer_Metrics::Metric_renderFrame* const metric) = 0;
        virtual bool release(bool gpuContextLost) = 0;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_STAGE_H_)
