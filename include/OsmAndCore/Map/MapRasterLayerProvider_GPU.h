#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_GPU_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_GPU_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapRasterLayerProvider.h>

namespace OsmAnd
{
    class MapPrimitivesProvider;

    class MapRasterLayerProvider_GPU_P;
    class OSMAND_CORE_API MapRasterLayerProvider_GPU : public MapRasterLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapRasterLayerProvider_GPU);
    private:
    protected:
    public:
        MapRasterLayerProvider_GPU(
            const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider,
            const bool fillBackground = true,
            const bool forceObtainDataAsync = false);
        virtual ~MapRasterLayerProvider_GPU();
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_GPU_H_)
