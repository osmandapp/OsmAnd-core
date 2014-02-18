#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRendererStage.h"
#include "AtlasMapRendererInternalState_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRenderer_OpenGL;
    class GPUAPI_OpenGL;

    class AtlasMapRendererStage_OpenGL : public MapRendererStage
    {
        Q_DISABLE_COPY(AtlasMapRendererStage_OpenGL);
    public:
        AtlasMapRendererStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererStage_OpenGL();

        AtlasMapRenderer_OpenGL* getRenderer() const;
        GPUAPI_OpenGL* getGPUAPI() const;
        const AtlasMapRendererInternalState_OpenGL& internalState;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_OPENGL_H_)
