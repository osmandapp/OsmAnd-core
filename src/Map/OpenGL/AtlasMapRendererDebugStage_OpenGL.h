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
        GLname _vaoPoint2D;
        GLname _vboPoint2D;
        GLname _iboPoint2D;
        struct ProgramPoint2D {
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
                    GLlocation point;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLlocation color;
                } param;
            } fs;
        } _programPoint2D;
        bool initializePoints2D();
        bool renderPoints2D();
        bool releasePoints2D(bool gpuContextLost);
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
        bool initializeRects2D();
        bool renderRects2D();
        bool releaseRects2D(bool gpuContextLost);

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
        bool initializeLines2D();
        bool renderLines2D();
        bool releaseLines2D(bool gpuContextLost);

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
        bool initializeLines3D();
        bool renderLines3D();
        bool releaseLines3D(bool gpuContextLost);

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
        bool initializeQuads3D();
        bool renderQuads3D();
        bool releaseQuads3D(bool gpuContextLost);
    public:
        AtlasMapRendererDebugStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererDebugStage_OpenGL();

        virtual bool initialize();
        virtual bool render(IMapRenderer_Metrics::Metric_renderFrame* const metric);
        virtual bool release(bool gpuContextLost);
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_)
