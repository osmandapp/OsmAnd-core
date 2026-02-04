#ifndef _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_
#define _OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapRasterLayerProvider.h>
#include <OsmAndCore/Map/MapRasterLayerProvider_Metrics.h>

namespace OsmAnd
{
    class MapPrimitivesProvider;
    class MapRasterLayerProvider_Software_P;
    class OSMAND_CORE_API MapRasterLayerProvider_Software : public MapRasterLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(MapRasterLayerProvider_Software);
    private:
    protected:
    public:
        MapRasterLayerProvider_Software(
            const std::shared_ptr<MapPrimitivesProvider>& primitivesProvider,
            const bool fillBackground = true,
            const bool forceObtainDataAsync = false,
            const bool adjustToDetailedZoom = false);
        virtual ~MapRasterLayerProvider_Software();
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_)
