#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "AtlasMapRendererSymbolsStage.h"
#include "AtlasMapRendererStageHelper_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererSymbolsStage_OpenGL 
        : public AtlasMapRendererSymbolsStage
        , private AtlasMapRendererStageHelper_OpenGL
    {
    private:
    protected:
        void renderBillboardSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            int& lastUsedProgram);

        GLname _billboardRasterSymbolVAO;
        GLname _billboardRasterSymbolVBO;
        GLname _billboardRasterSymbolIBO;
        struct BillboardRasterSymbolProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;
                    GLlocation mOrthographicProjection;
                    GLlocation viewport;

                    // Per-symbol data
                    GLlocation symbolOffsetFromTarget;
                    GLlocation symbolSize;
                    GLlocation distanceFromCamera;
                    GLlocation onScreenOffset;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                    GLlocation modulationColor;
                } param;
            } fs;
        } _billboardRasterProgram;
        void initializeBillboardRaster();
        void renderBillboardRasterSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            int& lastUsedProgram);
        void releaseBillboardRaster();

        void initializeOnPath();
        void renderOnPathSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            int& lastUsedProgram);
        void releaseOnPath();

        struct Glyph
        {
            GLlocation anchorPoint;
            GLlocation width;
            GLlocation angle;
            GLlocation widthOfPreviousN;
            GLlocation widthN;
        };
        GLname _onPathSymbol2dVAO;
        GLname _onPathSymbol2dVBO;
        GLname _onPathSymbol2dIBO;
        struct OnPathSymbol2dProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation glyphIndex;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mOrthographicProjection;

                    // Per-symbol data
                    GLlocation glyphHeight;
                    GLlocation distanceFromCamera;

                    // Per-glyph data
                    QVector<Glyph> glyphs;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                    GLlocation modulationColor;
                } param;
            } fs;
        } _onPath2dProgram;
        GLint _onPathSymbol2dMaxGlyphsPerDrawCall;
        void initializeOnPath2D();
        void renderOnPath2dSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            int& lastUsedProgram);
        void releaseOnPath2D();

        GLname _onPathSymbol3dVAO;
        GLname _onPathSymbol3dVBO;
        GLname _onPathSymbol3dIBO;
        struct OnPathSymbol3dProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation glyphIndex;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;

                    // Per-symbol data
                    GLlocation glyphHeight;
                    GLlocation zDistanceFromCamera;

                    // Per-glyph data
                    QVector<Glyph> glyphs;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                    GLlocation modulationColor;
                } param;
            } fs;
        } _onPath3dProgram;
        GLint _onPathSymbol3dMaxGlyphsPerDrawCall;
        void initializeOnPath3D();
        void renderOnPath3dSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            int& lastUsedProgram);
        void releaseOnPath3D();

        void renderOnSurfaceSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            int& lastUsedProgram);

        GLname _onSurfaceRasterSymbolVAO;
        GLname _onSurfaceRasterSymbolVBO;
        GLname _onSurfaceRasterSymbolIBO;
        struct OnSurfaceSymbolProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexTexCoords;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;

                    // Per-symbol data
                    GLlocation symbolOffsetFromTarget;
                    GLlocation direction;
                    GLlocation symbolSize;
                    GLlocation zDistanceFromCamera;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation sampler;
                    GLlocation modulationColor;
                } param;
            } fs;
        } _onSurfaceRasterProgram;
        void initializeOnSurfaceRaster();
        void renderOnSurfaceRasterSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            int& lastUsedProgram);
        void releaseOnSurfaceRaster();

        struct OnSurfaceVectorProgram {
            GLname id;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexColor;
                } in;

                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                    GLlocation mModelViewProjection;
                    GLlocation zDistanceFromCamera;
                    GLlocation modulationColor;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data

                    // Per-symbol data
                } param;
            } fs;
        } _onSurfaceVectorProgram;
        void initializeOnSurfaceVector();
        void renderOnSurfaceVectorSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            int& lastUsedProgram);
        void releaseOnSurfaceVector();
    public:
        AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRendererSymbolsStage_OpenGL();

        virtual void initialize();
        virtual void render();
        virtual void release();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_)
