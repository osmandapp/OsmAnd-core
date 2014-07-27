#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_SOFTWARE_P_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_SOFTWARE_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapRasterBitmapTileProvider.h"
#include "BinaryMapRasterBitmapTileProvider_P.h"

class SkBitmap;

namespace OsmAnd
{
    class BinaryMapPrimitivesTile;

    class BinaryMapRasterBitmapTileProvider_Software;
    class BinaryMapRasterBitmapTileProvider_Software_P Q_DECL_FINAL : public BinaryMapRasterBitmapTileProvider_P
    {
    private:
    protected:
        BinaryMapRasterBitmapTileProvider_Software_P(BinaryMapRasterBitmapTileProvider_Software* owner);
    public:
        virtual ~BinaryMapRasterBitmapTileProvider_Software_P();

        ImplementationInterface<BinaryMapRasterBitmapTileProvider_Software> owner;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController);

    friend class OsmAnd::BinaryMapRasterBitmapTileProvider_Software;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_SOFTWARE_P_H_)
