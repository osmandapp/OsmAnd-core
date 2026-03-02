#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "MapRendererStage.h"
#include "AtlasMapRendererConfiguration.h"
#include "AtlasMapRendererStageHelper.h"

namespace OsmAnd
{
    class AtlasMapRenderer;
    namespace AtlasMapRenderer_Metrics
    {
        struct Metric_renderFrame;
    }

    class AtlasMapRendererStage
        : public MapRendererStage
        , protected AtlasMapRendererStageHelper
    {
        Q_DISABLE_COPY_AND_MOVE(AtlasMapRendererStage);
    private:
    protected:
        std::shared_ptr<const GPUAPI::ResourceInGPU> captureElevationDataResource(
            TileId normalizedTileId,
            ZoomLevel zoomLevel,
            std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource = nullptr,
            bool* isNotReady = nullptr) const;

        OsmAnd::ZoomLevel getElevationData(
            TileId normalizedTileId,
            ZoomLevel zoomLevel,
            PointF& offsetInTileN,
            std::shared_ptr<const IMapElevationDataProvider::Data>* pOutSource = nullptr,
            bool* isNotReady = nullptr) const;

    public:
        explicit AtlasMapRendererStage(AtlasMapRenderer* renderer);
        ~AtlasMapRendererStage() override;

        enum class InitSymbolType : int
        {
            BillboardRaster,
            OnPath2D,
            OnPath3D,
            OnSurfaceRaster,
            OnSurfaceVector,
            Model3D,
            VisibilityCheck,
            Complete,
            Incomplete
        };

        enum class InitDebugSymbolType : int
        {
            Points2D,
            Rects2D,
            Lines2D,
            Lines3D,
            Quads3D,
            Complete,
            Incomplete
        };

        enum class Init3DObjectsType : int
        {
            Objects3DDepth,
            Objects3DColor,
            Complete,
            Incomplete
        };

        SWIG_OMIT(Q_REQUIRED_RESULT) const AtlasMapRendererConfiguration& getCurrentConfiguration() const;
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_STAGE_H_)
