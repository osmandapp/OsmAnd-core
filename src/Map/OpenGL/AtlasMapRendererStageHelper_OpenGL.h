#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_HELPER_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_HELPER_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererInternalState_OpenGL.h"
#include "OpenGL/GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererStage;
    class AtlasMapRenderer_OpenGL;

    class AtlasMapRendererStageHelper_OpenGL
    {
        Q_DISABLE_COPY_AND_MOVE(AtlasMapRendererStageHelper_OpenGL);

    private:
        const AtlasMapRendererStage* const _stage;
    protected:
        AtlasMapRendererStageHelper_OpenGL(const AtlasMapRendererStage* const stage);
    public:
        virtual ~AtlasMapRendererStageHelper_OpenGL();

        AtlasMapRenderer_OpenGL* getRenderer() const;
        GPUAPI_OpenGL* getGPUAPI() const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_HELPER_OPENGL_H_)
