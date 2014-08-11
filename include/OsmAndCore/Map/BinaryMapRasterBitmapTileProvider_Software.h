#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_SOFTWARE_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_SOFTWARE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider.h>
#include <OsmAndCore/Map/BinaryMapRasterBitmapTileProvider_Metrics.h>

namespace OsmAnd
{
    class BinaryMapPrimitivesProvider;
    class BinaryMapRasterBitmapTileProvider_Software_P;
    class OSMAND_CORE_API BinaryMapRasterBitmapTileProvider_Software : public BinaryMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapRasterBitmapTileProvider_Software);
    private:
    protected:
    public:
        BinaryMapRasterBitmapTileProvider_Software(const std::shared_ptr<BinaryMapPrimitivesProvider>& primitivesProvider);
        virtual ~BinaryMapRasterBitmapTileProvider_Software();
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_SOFTWARE_H_)
