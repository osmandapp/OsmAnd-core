#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/BinaryMapRasterLayerProvider.h>
#include <OsmAndCore/Map/BinaryMapRasterLayerProvider_Metrics.h>

namespace OsmAnd
{
    class BinaryMapPrimitivesProvider;
    class BinaryMapRasterLayerProvider_Software_P;
    class OSMAND_CORE_API BinaryMapRasterLayerProvider_Software : public BinaryMapRasterLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapRasterLayerProvider_Software);
    private:
    protected:
    public:
        BinaryMapRasterLayerProvider_Software(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider);
        virtual ~BinaryMapRasterLayerProvider_Software();
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_SOFTWARE_H_)
