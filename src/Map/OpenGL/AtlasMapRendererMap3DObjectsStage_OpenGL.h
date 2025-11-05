#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
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

        int drawResource(const TileId& id, ZoomLevel z, const std::shared_ptr<MapRenderer3DObjectsResource>& res, QSet<uint64_t>& drawnBboxHashes);

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
                } in;

                // Params
                struct
                {
                    GLlocation mPerspectiveProjectionView;
                    GLlocation metersPerUnit;
                    GLlocation color;
                    GLlocation alpha;
                    GLlocation target31;
                    GLlocation zoomLevel;
                    GLlocation lightDirection;
                    GLlocation ambient;
                } param;
            } vs;
        } _program;

        bool initializeProgram();

    public:
        explicit AtlasMapRendererMap3DObjectsStage_OpenGL(AtlasMapRenderer_OpenGL* renderer);
        ~AtlasMapRendererMap3DObjectsStage_OpenGL() override;

        bool initialize() override;
        StageResult render(IMapRenderer_Metrics::Metric_renderFrame* const metric) override;
        bool release(bool gpuContextLost) override;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_MAP3DOBJECTS_STAGE_OPENGL_H_)


