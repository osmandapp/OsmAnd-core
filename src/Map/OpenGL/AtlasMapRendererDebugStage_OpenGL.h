#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_

#include "stdlib_common.h"
#include <tuple>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererDebugStage.h"
#include "AtlasMapRendererStageHelper_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererDebugStage_OpenGL
        : public AtlasMapRendererDebugStage
        , private AtlasMapRendererStageHelper_OpenGL
    {
    private:
    protected:
        GLname _vaoRect2D;
        GLname _vboRect2D;
        GLname _iboRect2D;
        struct ProgramRect2D {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mProjectionViewModel;
                    GLlocation rect;
                    GLlocation angle;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLlocation color;
                } param;
            } fs;
        } _programRect2D;
        void initializeRects2D();
        void renderRects2D();
        void releaseRects2D();

        GLname _vaoLine2D;
        GLname _vboLine2D;
        GLname _iboLine2D;
        struct ProgramLine2D {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mProjectionViewModel;
                    GLlocation v0;
                    GLlocation v1;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLlocation color;
                } param;
            } fs;
        } _programLine2D;
        void initializeLines2D();
        void renderLines2D();
        void releaseLines2D();

        GLname _vaoLine3D;
        GLname _vboLine3D;
        GLname _iboLine3D;
        struct ProgramLine3D {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mProjectionViewModel;
                    GLlocation v0;
                    GLlocation v1;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLlocation color;
                } param;
            } fs;
        } _programLine3D;
        void initializeLines3D();
        void renderLines3D();
        void releaseLines3D();

        GLname _vaoQuad3D;
        GLname _vboQuad3D;
        GLname _iboQuad3D;
        struct ProgramQuad3D {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mProjectionViewModel;
                    GLlocation v0;
                    GLlocation v1;
                    GLlocation v2;
                    GLlocation v3;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLlocation color;
                } param;
            } fs;
        } _programQuad3D;
        void initializeQuads3D();
        void renderQuads3D();
        void releaseQuads3D();
    public:
        AtlasMapRendererDebugStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererDebugStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_)