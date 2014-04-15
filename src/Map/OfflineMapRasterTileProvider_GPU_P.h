#ifndef _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_P_H_
#define _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_P_H_

#include "stdlib_common.h"
#include <functional>
#include <array>

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapBitmapTileProvider.h"
#include "IRetainableResource.h"

class SkBitmap;

namespace OsmAnd
{
    class OfflineMapDataTile;

    class OfflineMapRasterTileProvider_GPU;
    class OfflineMapRasterTileProvider_GPU_P
    {
    private:
    protected:
        OfflineMapRasterTileProvider_GPU_P(OfflineMapRasterTileProvider_GPU* owner, const uint32_t outputTileSize, const float density);

        class Tile : public MapBitmapTile, public IRetainableResource
        {
            Q_DISABLE_COPY(Tile);
        private:
            const std::shared_ptr<const OfflineMapDataTile> _dataTile;
        protected:
        public:
            Tile(SkBitmap* bitmap, const std::shared_ptr<const OfflineMapDataTile>& dataTile);
            virtual ~Tile();

            const std::shared_ptr<const OfflineMapDataTile>& dataTile;

            virtual void releaseNonRetainedData();
        };

        ImplementationInterface<OfflineMapRasterTileProvider_GPU> owner;
        const uint32_t outputTileSize;
        const float density;

        bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController);

        ZoomLevel getMinZoom() const;
        ZoomLevel getMaxZoom() const;
    public:
        virtual ~OfflineMapRasterTileProvider_GPU_P();

    friend class OsmAnd::OfflineMapRasterTileProvider_GPU;
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_P_H_)
