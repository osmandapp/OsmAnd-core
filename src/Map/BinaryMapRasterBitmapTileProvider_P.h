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
#include "BinaryMapRasterBitmapTileProvider_Metrics.h"

namespace OsmAnd
{
    class MapRasterizer;

    class BinaryMapRasterBitmapTileProvider;
    class BinaryMapRasterBitmapTileProvider_P
    {
        Q_DISABLE_COPY_AND_MOVE(BinaryMapRasterBitmapTileProvider_P);
    private:
    protected:
        BinaryMapRasterBitmapTileProvider_P(BinaryMapRasterBitmapTileProvider* const owner);

        virtual void initialize();
        std::unique_ptr<MapRasterizer> _mapRasterizer;
    public:
        virtual ~BinaryMapRasterBitmapTileProvider_P();

        ImplementationInterface<BinaryMapRasterBitmapTileProvider> owner;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController);

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            BinaryMapRasterBitmapTileProvider_Metrics::Metric_obtainData* const metric,
            const IQueryController* const queryController) = 0;

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;

    friend class OsmAnd::BinaryMapRasterBitmapTileProvider;
    };
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_P_H_)
