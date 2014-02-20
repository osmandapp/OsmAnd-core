#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_

#include "stdlib_common.h"
#include <tuple>

#include <glm/glm.hpp>

#include "QtExtensions.h"
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererStage_OpenGL.h"
#include "GPUAPI_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererDebugStage_OpenGL : public AtlasMapRendererStage_OpenGL
    {
    private:
    protected:
        typedef std::tuple<AreaF, uint32_t, float> Rect2D;
        QList<Rect2D> _rects2D;
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

        typedef std::pair< QVector<glm::vec2>, uint32_t> Line2D;
        QList<Line2D> _lines2D;
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

        typedef std::pair< QVector<glm::vec3>, uint32_t> Line3D;
        QList<Line3D> _lines3D;
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

        typedef std::tuple< glm::vec3, glm::vec3, glm::vec3, glm::vec3, uint32_t> Quad3D;
        QList<Quad3D> _quads3D;
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

        void clear();
        void addRect2D(const AreaF& rect, const uint32_t argbColor, const float angle = 0.0f);
        void addLine2D(const QVector<glm::vec2>& line, const uint32_t argbColor);
        void addLine3D(const QVector<glm::vec3>& line, const uint32_t argbColor);
        void addQuad3D(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const uint32_t argbColor);
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_)