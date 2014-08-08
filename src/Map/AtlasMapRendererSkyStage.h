#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SKY_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SKY_STAGE_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage.h"

namespace OsmAnd
{
    class AtlasMapRendererSkyStage : public AtlasMapRendererStage
    {
    private:
    protected:
    public:
        AtlasMapRendererSkyStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererSkyStage();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SKY_STAGE_H_)