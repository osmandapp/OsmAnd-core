#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_H_

#include "AtlasMapRendererStage.h"

namespace OsmAnd
{
    class AtlasMapRendererMap3DObjectsStage : public AtlasMapRendererStage
    {
        Q_DISABLE_COPY_AND_MOVE(AtlasMapRendererMap3DObjectsStage);
    public:
        explicit AtlasMapRendererMap3DObjectsStage(AtlasMapRenderer* const renderer)
            : AtlasMapRendererStage(renderer)
        {
        }
        ~AtlasMapRendererMap3DObjectsStage() override = default;

        virtual bool initialize() override = 0;
        virtual StageResult render(IMapRenderer_Metrics::Metric_renderFrame* const metric) override = 0;
        virtual bool release(bool gpuContextLost) override = 0;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_H_)


