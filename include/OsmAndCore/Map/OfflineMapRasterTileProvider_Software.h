#ifndef _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_SOFTWARE_H_
#define _OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_SOFTWARE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QMutex>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/IMapRasterBitmapTileProvider.h>

namespace OsmAnd
{
    class OfflineMapDataProvider;
    class OfflineMapRasterTileProvider_Software_P;
    class OSMAND_CORE_API OfflineMapRasterTileProvider_Software : public IMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY(OfflineMapRasterTileProvider_Software);
    private:
        PrivateImplementation<OfflineMapRasterTileProvider_Software_P> _p;
    protected:
    public:
        OfflineMapRasterTileProvider_Software(const std::shared_ptr<OfflineMapDataProvider>& dataProvider, const uint32_t outputTileSize = 256, const float density = 1.0f);
        virtual ~OfflineMapRasterTileProvider_Software();

        const std::shared_ptr<OfflineMapDataProvider> dataProvider;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_OFFLINE_MAP_RASTER_TILE_PROVIDER_SOFTWARE_H_)
