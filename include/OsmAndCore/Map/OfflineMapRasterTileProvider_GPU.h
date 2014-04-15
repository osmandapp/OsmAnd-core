#ifndef _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_H_
#define _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

namespace OsmAnd
{
    class OfflineMapDataProvider;
    class OfflineMapRasterTileProvider_GPU_P;
    class OSMAND_CORE_API OfflineMapRasterTileProvider_GPU : public IMapBitmapTileProvider
    {
        Q_DISABLE_COPY(OfflineMapRasterTileProvider_GPU);
    private:
        PrivateImplementation<OfflineMapRasterTileProvider_GPU_P> _p;
    protected:
    public:
        OfflineMapRasterTileProvider_GPU(const std::shared_ptr<OfflineMapDataProvider>& dataProvider, const uint32_t outputTileSize = 256, const float density = 1.0f);
        virtual ~OfflineMapRasterTileProvider_GPU();

        const std::shared_ptr<OfflineMapDataProvider> dataProvider;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController);
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_H_)
