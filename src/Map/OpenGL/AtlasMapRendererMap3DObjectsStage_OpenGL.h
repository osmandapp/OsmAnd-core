#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererMap3DObjectsStage.h"
#include "AtlasMapRendererStageHelper_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererMap3DObjectsStage_OpenGL
        : public AtlasMapRendererMap3DObjectsStage
        , private AtlasMapRendererStageHelper_OpenGL
    {
        using AtlasMapRendererStageHelper_OpenGL::getRenderer;

    private:
        GLname _vao;
        GLname _vbo;
        GLname _program;

        bool initializeProgram();

    public:
        explicit AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer);
        ~AtlasMapRendererMap3DObjectsStage_OpenGL() override;

        bool initialize() override;
        bool render(IMapRenderer_Metrics::Metric_renderFrame* const metric) override;
        bool release(bool gpuContextLost) override;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_)


