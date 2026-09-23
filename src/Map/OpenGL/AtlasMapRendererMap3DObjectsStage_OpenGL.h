#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include <OsmAndCore/PointsAndAreas.h>
#include <QtCore/QVector>
#include "AtlasMapRendererMap3DObjectsStage.h"
#include "AtlasMapRendererStageHelper_OpenGL.h"

namespace OsmAnd
{
    class AtlasMapRendererMap3DObjectsStage_OpenGL
        : public AtlasMapRendererMap3DObjectsStage
        , private AtlasMapRendererStageHelper_OpenGL
    {
        using AtlasMapRendererStageHelper_OpenGL::getRenderer;

    private:
        GLname _vao;
        GLname _colorVao;
        GLname _depthVao;
        GLname _shadowVao;

        Init3DObjectsType _init3DObjectsType;

        struct Model3DProgram
        {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

            // Vertex data
            struct
            {
                // Input data
                struct
                {
                    GLlocation location31;
                    GLlocation sizes;
                    GLlocation heights;
                    GLlocation normal;
                    GLlocation color;
                } in;

                // Params
                struct
                {
                    GLlocation mPerspectiveProjectionView;
                    GLlocation resultScale;
                    GLlocation target31;
                    GLlocation zoomLevel;
                    GLlocation metersPerUnit;
                    GLlocation zScaleFactor;
                    GLlocation lightMatrix;
                    GLlocation shadowNormalOffset;
                } param;
            } vs;
            // Vertex data
            struct
            {
                // Params
                struct
                {
                    GLlocation alpha;
                    GLlocation fadeHeight;
                    GLlocation cameraPosition;
                    GLlocation lightDirection;
                    GLlocation shadowMap;
                    GLlocation shadowStrength;
                } param;
            } fs;
            // Shadow map sampling compiles only with GLSL ES 3.0 or desktop GLSL 1.30+
            bool withShadowMap;
        } _program;
        Model3DProgram _colorProgram;
        Model3DProgram _depthProgram;

        // Buildings projected onto the ground along the sun direction
        struct ShadowProgram
        {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

            struct
            {
                struct
                {
                    GLlocation location31;
                    GLlocation heights;
                } in;

                struct
                {
                    GLlocation mPerspectiveProjectionView;
                    GLlocation resultScale;
                    GLlocation target31;
                    GLlocation zoomLevel;
                    GLlocation metersPerUnit;
                    GLlocation zScaleFactor;
                    GLlocation lightDirection;
                } param;
            } vs;

            struct
            {
                struct
                {
                    GLlocation shadowAlpha;
                    GLlocation noiseSeed;
                } param;
            } fs;
        } _shadowProgram;

        // Depth of the buildings as seen from the sun: shadows cast onto walls and roofs
        GLuint _shadowMapTexture;
        GLuint _shadowMapFramebuffer;
        bool _shadowMapFailed;
        float _shadowMapStrength;
        float _shadowMapNormalOffset;
        glm::mat4 _shadowMapMatrix;

        QList<std::shared_ptr<const GPUAPI::MeshInGPU>> resourcesInGPU;
        QMap<int, QSet<TileId>> _firstTiles;
        QMap<int, QSet<TileId>> _firstSpace;
        QMap<int, QSet<TileId>> _secondTiles;
        QMap<int, QSet<TileId>> _secondSpace;
        QMap<int, QSet<TileId>>* actualTiles;
        QMap<int, QSet<TileId>>* actualSpace;
        QMap<int, QSet<TileId>>* oldTiles;
        QMap<int, QSet<TileId>>* oldSpace;

        bool initializeSimpleProgram();
        bool initializeColorProgram();
        bool initializeDepthProgram();
        bool initializeShadowProgram();
        void occupySpace(TileId tileIdN, int zoomLevel, int minZoomLevel,
            QMap<int, QSet<TileId>>& presentTiles, QMap<int, QSet<TileId>>& occupiedSpace) const;
        bool spaceAlreadyOccupied(TileId tileIdN, int zoomLevel, QMap<int, QSet<TileId>>& presentTiles,
            QMap<int, QSet<TileId>>& occupiedSpace, bool* exact = nullptr) const;
        void getResourcesInGPU(const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection,
            const int viewableDetalizationLevel, bool& highDetalizationLevel,
            const int64_t appearTime, bool& shouldInvalidateFrame);
        StageResult renderDepth(bool primaryOnly);
        StageResult renderShadows();
        bool renderShadowMap();
        void releaseShadowMap(bool gpuContextLost);
        void setupShadowMapSampling(const Model3DProgram& program, float strength);
        StageResult renderSimple(bool primaryOnly);
        StageResult renderColor(bool primaryOnly, int64_t currentTime);
        std::shared_ptr<const GPUAPI::MeshInGPU> captureResourceInGPU(
            const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection,
            TileId normalizedTileId,
            ZoomLevel zoomLevel) const;
    public:
        explicit AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer);
        ~AtlasMapRendererMap3DObjectsStage_OpenGL() override;

        bool initialize() override;
        StageResult render(IMapRenderer_Metrics::Metric_renderFrame* const metric) override;
        bool release(bool gpuContextLost) override;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_)


