#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_3D_MODELS_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_3D_MODELS_STAGE_H_

#include "AtlasMapRendererStage.h"

namespace OsmAnd
{
    class AtlasMapRenderer3DModelsStage : public AtlasMapRendererStage
    {
    private:
    protected:
    public:
        AtlasMapRenderer3DModelsStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRenderer3DModelsStage();
    };   
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_3D_MODELS_STAGE_H_)