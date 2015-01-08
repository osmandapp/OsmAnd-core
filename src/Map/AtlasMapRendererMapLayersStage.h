#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_H_

#include "stdlib_common.h"
#include <array>

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage.h"

namespace OsmAnd
{
    class AtlasMapRenderer;

    class AtlasMapRendererMapLayersStage : public AtlasMapRendererStage
    {
    private:
    protected:
    public:
        AtlasMapRendererMapLayersStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererMapLayersStage();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP_LAYERS_STAGE_H_)
