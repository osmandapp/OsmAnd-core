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

        struct ElevationData
        {
            std::shared_ptr<const GPUAPI::ResourceInGPU> resource;
            ZoomLevel zoom;
            TileId tileIdN;
            PointF texCoordsOffset;
            PointF texCoordsScale;
            bool hasData;
        };

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
                    GLlocation normal;
                    GLlocation color;
                } in;

                // Params
                struct
                {
                    GLlocation mPerspectiveProjectionView;
                    GLlocation resultScale;
                    GLlocation alpha;
                    GLlocation target31;
                    GLlocation zoomLevel;
                    GLlocation tileZoomLevel;
                    GLlocation lightDirection;
                    GLlocation ambient;
                    // Elevation data
                    GLlocation elevation_dataSampler;
                    struct
                    {
                        GLlocation txOffsetAndScale;
                    } elevationLayer;
                    GLlocation elevationLayerDataPlace;
                    GLlocation elevation_scale;
                    GLlocation elevationTileCoords31;
                    GLlocation elevationTileZoomLevel;
                } param;
            } vs;
        } _program;
        Model3DProgram _depthProgram;

        ElevationData findElevationData(const TileId& tileIdN, ZoomLevel buildingZoom);

        void prepareElevation(const TileId& id, ZoomLevel z, const AtlasMapRendererMap3DObjectsStage_OpenGL::Model3DProgram& program);

        void configureElevationData(
            const std::shared_ptr<const GPUAPI::ResourceInGPU>& elevationDataResource,
            const TileId tileIdN,
            const ZoomLevel zoomLevel,
            const PointF& texCoordsOffsetN,
            const PointF& texCoordsScaleN,
            const Model3DProgram& program);
        void cancelElevation(const Model3DProgram& program);

        bool initializeColorProgram();
        bool initializeDepthProgram();
        void prepareDrawObjects(QSet<std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData>>& collectedResources);
        StageResult renderDepth(QSet<std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData>>& collectedResources);
        StageResult renderColor(QSet<std::shared_ptr<GPUAPI::MapRenderer3DBuildingGPUData>>& collectedResources);

    public:
        explicit AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer);
        ~AtlasMapRendererMap3DObjectsStage_OpenGL() override;

        bool initialize() override;
        StageResult render(IMapRenderer_Metrics::Metric_renderFrame* const metric) override;
        bool release(bool gpuContextLost) override;

        bool isDepthPrepassRequired() const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_)


