#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_GPU_P_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_GPU_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IRasterMapLayerProvider.h"
#include "MapRasterLayerProvider_P.h"
#include "MapRasterizer.h"

namespace OsmAnd
{
    class MapRasterLayerProvider_GPU;
    class MapRasterLayerProvider_GPU_P Q_DECL_FINAL : public MapRasterLayerProvider_P
    {
    private:
    protected:
        MapRasterLayerProvider_GPU_P(MapRasterLayerProvider_GPU* owner);

        virtual sk_sp<SkImage> rasterize(
            const MapRasterLayerProvider::Request& request,
            const std::shared_ptr<const MapPrimitivesProvider::Data>& primitivesTile,
            MapRasterLayerProvider_Metrics::Metric_obtainData* const metric);
    public:
        virtual ~MapRasterLayerProvider_GPU_P();

        ImplementationInterface<MapRasterLayerProvider_GPU> owner;

    friend class OsmAnd::MapRasterLayerProvider_GPU;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_GPU_P_H_)
