#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_GPU_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_GPU_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/BinaryMapRasterLayerProvider.h>

namespace OsmAnd
{
    class BinaryMapPrimitivesProvider;

    class BinaryMapRasterLayerProvider_GPU_P;
    class OSMAND_CORE_API BinaryMapRasterLayerProvider_GPU : public BinaryMapRasterLayerProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapRasterLayerProvider_GPU);
    private:
    protected:
    public:
        BinaryMapRasterLayerProvider_GPU(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider);
        virtual ~BinaryMapRasterLayerProvider_GPU();
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_GPU_H_)
