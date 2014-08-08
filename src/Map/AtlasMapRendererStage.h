#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRendererStage.h"
#include "AtlasMapRendererConfiguration.h"
#include "AtlasMapRendererStageHelper.h"

namespace OsmAnd
{
    class AtlasMapRenderer;

    class AtlasMapRendererStage
        : public MapRendererStage
        , protected AtlasMapRendererStageHelper
    {
        Q_DISABLE_COPY(AtlasMapRendererStage);
    private:
    protected:
    public:
        AtlasMapRendererStage(AtlasMapRenderer* const renderer);
        virtual ~AtlasMapRendererStage();

        const AtlasMapRendererConfiguration& getCurrentConfiguration() const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_H_)
