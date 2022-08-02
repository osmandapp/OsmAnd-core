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
        using AtlasMapRendererStageHelper_OpenGL::getRenderer;

    private:
    protected:
        bool renderBillboardSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);

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
                    GLlocation cameraInWorld;
                    GLlocation target31;

                    // Per-tile data
                    GLlocation elevation_scale;

                    // Per-symbol data
                    GLlocation position31;
                    GLlocation offsetInTile;
                    GLlocation symbolSize;
                    GLlocation distanceFromCamera;
                    GLlocation onScreenOffset;
                    GLlocation elevationInMeters;
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
        bool initializeBillboardRaster();
        bool renderBillboardRasterSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);
        bool releaseBillboardRaster(bool gpuContextLost);

        bool initializeOnPath();
        bool renderOnPathSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);
        bool releaseOnPath(bool gpuContextLost);

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
        unsigned int _onPathSymbol2dMaxGlyphsPerDrawCall;
        bool initializeOnPath2D();
        bool initializeOnPath2DProgram(unsigned int maxGlyphsPerDrawCall);
        bool renderOnPath2dSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);
        bool releaseOnPath2D(bool gpuContextLost);

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
        unsigned int _onPathSymbol3dMaxGlyphsPerDrawCall;
        bool initializeOnPath3D();
        bool initializeOnPath3DProgram(unsigned int maxGlyphsPerDrawCall);
        bool renderOnPath3dSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);
        bool releaseOnPath3D(bool gpuContextLost);

        bool renderOnSurfaceSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);

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
        bool initializeOnSurfaceRaster();
        bool renderOnSurfaceRasterSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);
        bool releaseOnSurfaceRaster(bool gpuContextLost);

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
                    GLlocation tileId;
                    GLlocation lookupOffsetAndScale;
                    GLlocation elevation_scale;
                    GLlocation elevation_dataSampler;
                    GLlocation texCoordsOffsetAndScale;
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
        bool initializeOnSurfaceVector();
        bool renderOnSurfaceVectorSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType,
            GLname& lastUsedProgram);
        bool releaseOnSurfaceVector(bool gpuContextLost);

        // Terrain-related:
        bool applyTerrainVisibilityFiltering(
            const glm::vec3& positionOnScreen,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) const override;

        void configureElevationData(
            const OnSurfaceVectorProgram& program,
            const TileId tileId,
            const int elevationDataSamplerIndex);
    public:
        explicit AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer);
        ~AtlasMapRendererSymbolsStage_OpenGL() override;

        bool initialize() override;
        bool render(IMapRenderer_Metrics::Metric_renderFrame* metric) override;
        bool release(bool gpuContextLost) override;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_)
