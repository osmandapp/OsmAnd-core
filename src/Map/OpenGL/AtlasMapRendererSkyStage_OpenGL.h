#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SKY_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SKY_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage_OpenGL.h"
#include "GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererSkyStage_OpenGL : public AtlasMapRendererStage_OpenGL
    {
    private:
    protected:
        GLuint _skyplaneVAO;
        GLuint _skyplaneVBO;
        GLuint _skyplaneIBO;

        struct {
            GLuint id;

            struct {
                // Input data
                struct {
                    GLint vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mProjectionViewModel;
                    GLint planeSize;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLint skyColor;
                } param;
            } fs;
        } _program;
    public:
        AtlasMapRendererSkyStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererSkyStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SKY_STAGE_OPENGL_H_)