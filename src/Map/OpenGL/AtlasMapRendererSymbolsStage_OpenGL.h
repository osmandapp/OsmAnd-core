#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage_OpenGL.h"
#include "GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererSymbolsStage_OpenGL : public AtlasMapRendererStage_OpenGL
    {
    private:
    protected:
        GLuint _pinnedSymbolVAO;
        GLuint _pinnedSymbolVBO;
        GLuint _pinnedSymbolIBO;
        struct {
            GLuint id;

            struct {
                // Input data
                struct {
                    GLint vertexPosition;
                    GLint vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mPerspectiveProjectionView;
                    GLint mOrthographicProjection;
                    GLint viewport;

                    // Per-symbol data
                    GLint symbolOffsetFromTarget;
                    GLint symbolSize;
                    GLint distanceFromCamera;
                    GLint onScreenOffset;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLint sampler;
                } param;
            } fs;
        } _pinnedSymbolProgram;
        void initializePinned();
        void releasePinned();

        GLuint _symbolOnPathVAO;
        GLuint _symbolOnPathVBO;
        GLuint _symbolOnPathIBO;
        struct {
            GLuint id;

            struct {
                // Input data
                struct {
                    GLint vertexPosition;
                    GLint vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mPerspectiveProjectionView;
                    GLint mOrthographicProjection;
                    GLint viewport;

                    // Per-symbol data
                    GLint symbolOffsetFromTarget;
                    GLint symbolSize;
                    GLint distanceFromCamera;
                    GLint onScreenOffset;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLint sampler;
                } param;
            } fs;
        } _symbolOnPathProgram;
        void initializeOnPath();
        void releaseOnPath();
    public:
        AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererSymbolsStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_)