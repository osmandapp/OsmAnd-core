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
        GLname _depthVao;

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
                    GLlocation height;
                    GLlocation terrainHeight;
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
                } param;
            } vs;
            // Vertex data
            struct
            {
                // Params
                struct
                {
                    GLlocation alpha;
                    GLlocation cameraPosition;
                    GLlocation lightDirection;
                } param;
            } fs;
        } _program;
        Model3DProgram _depthProgram;

        QList<std::shared_ptr<const GPUAPI::MeshInGPU>> resourcesInGPU;

        bool initializeColorProgram();
        bool initializeDepthProgram();
        void occupySpace(TileId tileIdN, int zoomLevel, int minZoomLevel,
            QMap<int, QSet<TileId>>& presentTiles, QMap<int, QSet<TileId>>& occupiedSpace) const;
        bool spaceAlreadyOccupied(TileId tileIdN, int zoomLevel,
            QMap<int, QSet<TileId>>& presentTiles, QMap<int, QSet<TileId>>& occupiedSpace) const;
        void getResourcesInGPU(
            const std::shared_ptr<const IMapRendererResourcesCollection>& resourcesCollection, int detalizationLevel);
        StageResult renderDepth(bool primaryOnly);
        StageResult renderColor(bool primaryOnly);
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


