#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_GPU_P_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_GPU_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IRasterMapLayerProvider.h"
#include "BinaryMapRasterLayerProvider_P.h"
#include "MapRasterizer.h"

class SkBitmap;

namespace OsmAnd
{
    class BinaryMapRasterLayerProvider_GPU;
    class BinaryMapRasterLayerProvider_GPU_P Q_DECL_FINAL: public BinaryMapRasterLayerProvider_P
    {
    private:
    protected:
        BinaryMapRasterLayerProvider_GPU_P(BinaryMapRasterLayerProvider_GPU* owner);

        virtual std::shared_ptr<SkBitmap> rasterize(
            const TileId tileId,
            const ZoomLevel zoom,
            const std::shared_ptr<const BinaryMapPrimitivesProvider::Data>& primitivesTile,
            BinaryMapRasterLayerProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);
    public:
        virtual ~BinaryMapRasterLayerProvider_GPU_P();

        ImplementationInterface<BinaryMapRasterLayerProvider_GPU> owner;

    friend class OsmAnd::BinaryMapRasterLayerProvider_GPU;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_GPU_P_H_)
