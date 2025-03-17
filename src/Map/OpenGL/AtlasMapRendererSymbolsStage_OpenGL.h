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
        void createSubstages() override;

        GLname _lastUsedProgram;
        bool renderBillboardSymbol(
            const std::shared_ptr<const RenderableBillboardSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType);

        GLname _billboardRasterSymbolVAO;
        GLname _billboardRasterSymbolVBO;
        GLname _billboardRasterSymbolIBO;
        struct BillboardRasterSymbolProgram {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

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
                    GLlocation mRotate;
                    GLlocation resultScale;
                    GLlocation viewport;
                    GLlocation target31;

                    // Per-symbol data
                    GLlocation position31;
                    GLlocation symbolSize;
                    GLlocation distanceFromCamera;
                    GLlocation onScreenOffset;
                    GLlocation elevationInWorld;
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
            AlphaChannelType &currentAlphaChannelType);
        bool releaseBillboardRaster(bool gpuContextLost);

        bool initializeOnPath();
        bool renderOnPathSymbol(
            const std::shared_ptr<const RenderableOnPathSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType);
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
            QByteArray binaryCache;
            GLenum cacheFormat;

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
                    GLlocation resultScale;

                    // Per-symbol data
                    GLlocation glyphHeight;
                    GLlocation distanceFromCamera;
                    GLlocation currentOffset;

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
            AlphaChannelType &currentAlphaChannelType);
        bool releaseOnPath2D(bool gpuContextLost);

        GLname _onPathSymbol3dVAO;
        GLname _onPathSymbol3dVBO;
        GLname _onPathSymbol3dIBO;
        struct OnPathSymbol3dProgram {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

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
                    GLlocation resultScale;

                    // Per-symbol data
                    GLlocation glyphHeight;
                    GLlocation zDistanceFromCamera;
                    GLlocation currentOffset;

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
            AlphaChannelType &currentAlphaChannelType);
        bool releaseOnPath3D(bool gpuContextLost);

        bool renderOnSurfaceSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType);

        GLname _onSurfaceRasterSymbolVAO;
        GLname _onSurfaceRasterSymbolVBO;
        GLname _onSurfaceRasterSymbolIBO;
        struct OnSurfaceRasterProgram {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

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
                    GLlocation resultScale;

                    // Per-symbol data
                    GLlocation symbolOffsetFromTarget;
                    GLlocation direction;
                    GLlocation symbolSize;
                    GLlocation elevationInWorld;
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
            AlphaChannelType &currentAlphaChannelType);
        bool releaseOnSurfaceRaster(bool gpuContextLost);

        struct OnSurfaceVectorProgram {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                    GLlocation vertexColor;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation mPerspectiveProjectionView;
                    GLlocation resultScale;
                    GLlocation mModel;
                    GLlocation modulationColor;
                    GLlocation tileId;
                    GLlocation offsetInTile;
                    GLlocation lookupOffsetAndScale;
                    GLlocation cameraPositionAndZfar;
                    GLlocation elevation_scale;
                    GLlocation elevationInMeters;
                    GLlocation elevationFactor;
                    GLlocation elevation_dataSampler;
                    GLlocation texCoordsOffsetAndScale;
                    GLlocation elevationLayerDataPlace;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                    GLlocation startingDistance;
                } param;
            } fs;
        } _onSurfaceVectorProgram;
        bool initializeOnSurfaceVector();
        bool renderOnSurfaceVectorSymbol(
            const std::shared_ptr<const RenderableOnSurfaceSymbol>& renderable,
            AlphaChannelType &currentAlphaChannelType);
        bool releaseOnSurfaceVector(bool gpuContextLost);

        // Terrain-related:
        int _queryMaxCount;
        int _nextQueryIndex;
        int _queryResultsCount;
        QVector<bool> _queryResults;
        QHash<int64_t, int> _queryMapEven;
        QHash<int64_t, int> _queryMapOdd;
        QHash<int, bool> _queryResultsMap;
        float _querySizeFactor;
        GLname _visibilityCheckVAO;
        GLname _visibilityCheckVBO;
        struct VisibilityCheckProgram {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

            struct {
                // Input data
                struct {
                    GLlocation vertexPosition;
                } in;

                // Parameters
                struct {
                    // Common data
                    GLlocation firstPointPosition;
                    GLlocation secondPointPosition;
                    GLlocation thirdPointPosition;
                    GLlocation fourthPointPosition;
                    GLlocation cameraInWorld;
                    GLlocation mModelViewProjection;
                    GLlocation resultScale;
                } param;
            } vs;

            struct {
                // Parameters
                struct {
                    // Common data
                } param;
            } fs;
        } _visibilityCheckProgram;
        bool initializeVisibilityCheck();
        void clearTerrainVisibilityFiltering() override;
        int startTerrainVisibilityFiltering(
            const PointF& pointOnScreen,
            const glm::vec3& firstPointInWorld,
            const glm::vec3& secondPointInWorld,
            const glm::vec3& thirdPointInWorld,
            const glm::vec3& fourthPointInWorld) override;
        bool applyTerrainVisibilityFiltering(const int queryIndex,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) override;
        bool releaseVisibilityCheck(bool gpuContextLost);
        bool configureElevationData(
            const std::shared_ptr<const OsmAnd::GPUAPI::ResourceInGPU>& elevationDataResource,
            const OnSurfaceVectorProgram& program,
            const TileId tileIdN,
            const ZoomLevel zoomLevel,
            const PointF& texCoordsOffsetN,
            const PointF& texCoordsScaleN,
            const double tileSize,
            const int elevationDataSamplerIndex);
        void reportCommonParameters(QJsonObject& jsonObject, const RenderableSymbol& renderableSymbol);
    public:
        explicit AtlasMapRendererSymbolsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer);
        ~AtlasMapRendererSymbolsStage_OpenGL() override;

        bool initialize() override;
        bool preRender(QList< std::shared_ptr<const RenderableSymbol> >& preRenderableSymbols,
            AtlasMapRenderer_Metrics::Metric_renderFrame* metric) override;
        bool render(IMapRenderer_Metrics::Metric_renderFrame* metric) override;
        bool release(bool gpuContextLost) override;

    friend class AtlasMapRendererSymbolsStageModel3D_OpenGL;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_OPENGL_H_)
