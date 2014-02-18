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
        GLuint _vaoRect2D;
        GLuint _vboRect2D;
        GLuint _iboRect2D;
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
                    GLint rect;
                    GLint angle;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLint color;
                } param;
            } fs;
        } _programRect2D;
        void initializeRects2D();
        void renderRects2D();
        void releaseRects2D();

        typedef std::pair< QVector<glm::vec2>, uint32_t> Line2D;
        QList<Line2D> _lines2D;
        GLuint _vaoLine2D;
        GLuint _vboLine2D;
        GLuint _iboLine2D;
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
                    GLint v0;
                    GLint v1;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLint color;
                } param;
            } fs;
        } _programLine2D;
        void initializeLines2D();
        void renderLines2D();
        void releaseLines2D();

        typedef std::pair< QVector<glm::vec3>, uint32_t> Line3D;
        QList<Line3D> _lines3D;
        GLuint _vaoLine3D;
        GLuint _vboLine3D;
        GLuint _iboLine3D;
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
                    GLint v0;
                    GLint v1;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLint color;
                } param;
            } fs;
        } _programLine3D;
        void initializeLines3D();
        void renderLines3D();
        void releaseLines3D();
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
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_DEBUG_STAGE_OPENGL_H_)