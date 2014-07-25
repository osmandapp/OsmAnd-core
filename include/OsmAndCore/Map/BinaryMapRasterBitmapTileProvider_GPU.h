#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_GPU_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_GPU_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider.h>

namespace OsmAnd
{
    class BinaryMapPrimitivesProvider;

    class BinaryMapRasterBitmapTileProvider_GPU_P;
    class OSMAND_CORE_API BinaryMapRasterBitmapTileProvider_GPU : public BinaryMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY(BinaryMapRasterBitmapTileProvider_GPU);
    private:
    protected:
    public:
        BinaryMapRasterBitmapTileProvider_GPU(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider);
        virtual ~BinaryMapRasterBitmapTileProvider_GPU();
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_GPU_H_)
