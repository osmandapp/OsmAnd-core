#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_RASTER_MAP_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_RASTER_MAP_STAGE_H_

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

    class AtlasMapRendererRasterMapStage : public AtlasMapRendererStage
    {
    private:
    protected:
    public:
        AtlasMapRendererRasterMapStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererRasterMapStage();

        virtual void recreateTile() = 0;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_RASTER_MAP_STAGE_H_)