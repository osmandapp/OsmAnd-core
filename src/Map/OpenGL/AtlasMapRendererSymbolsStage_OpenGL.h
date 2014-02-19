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
        struct Glyph
        {
            GLint anchorPoint;
            GLint width;
            GLint angle;
            GLint widthOfPreviousN;
            GLint widthN;
        };
        struct {
            GLuint id;

            struct {
                // Input data
                struct {
                    GLint vertexPosition;
                    GLint glyphIndex;
                    GLint vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mOrthographicProjection;

                    // Per-symbol data
                    GLint glyphHeight;
                    GLint distanceFromCamera;

                    // Per-glyph data
                    QVector<Glyph>* pGlyphs;
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
        } _symbolOnPath2dProgram;
        QVector<Glyph> _symbolOnPath2dProgram_Glyphs;
        GLint _maxGlyphsPerDrawCallSOP2D;
        struct {
            GLuint id;

            struct {
                // Input data
                struct {
                    GLint vertexPosition;
                    GLint glyphIndex;
                    GLint vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLint mPerspectiveProjectionView;

                    // Per-symbol data
                    GLint glyphHeight;

                    // Per-glyph data
                    QVector<Glyph>* pGlyphs;
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
        } _symbolOnPath3dProgram;
        QVector<Glyph> _symbolOnPath3dProgram_Glyphs;
        GLint _maxGlyphsPerDrawCallSOP3D;
        void initializeOnPath();
        void initializeOnPath2D();
        void initializeOnPath3D();
        void releaseOnPath();
        void releaseOnPath2D();
        void releaseOnPath3D();
    public:
        AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererSymbolsStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_)