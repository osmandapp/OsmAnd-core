#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_HELPER_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_HELPER_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererInternalState.h"

namespace OsmAnd
{
    class AtlasMapRendererStage;
    class AtlasMapRenderer;

    class AtlasMapRendererStageHelper
    {
        Q_DISABLE_COPY(AtlasMapRendererStageHelper);

    private:
        AtlasMapRendererStage* const _stage;
    protected:
        AtlasMapRendererStageHelper(AtlasMapRendererStage* const stage);
    public:
        virtual ~AtlasMapRendererStageHelper();

        AtlasMapRenderer* getRenderer() const;
        const AtlasMapRendererInternalState& getInternalState() const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_HELPER_H_)
