#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_P_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"
#include <QMutex>
#include <QSet>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapRasterBitmapTileProvider.h"

namespace OsmAnd
{
    class BinaryMapRasterBitmapTileProvider;
    class BinaryMapRasterBitmapTileProvider_P
    {
        Q_DISABLE_COPY(BinaryMapRasterBitmapTileProvider_P);
    private:
    protected:
        BinaryMapRasterBitmapTileProvider_P(BinaryMapRasterBitmapTileProvider* const owner);
    public:
        virtual ~BinaryMapRasterBitmapTileProvider_P();

        ImplementationInterface<BinaryMapRasterBitmapTileProvider> owner;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<const MapTiledData>& outTiledData,
            const IQueryController* const queryController) = 0;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_P_H_)
