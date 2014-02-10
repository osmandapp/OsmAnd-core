#ifndef _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDER_H_
#define _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

namespace OsmAnd {

    class OnlineMapRasterTileProvider_P;
    class OSMAND_CORE_API OnlineMapRasterTileProvider : public IMapBitmapTileProvider
    {
        Q_DISABLE_COPY(OnlineMapRasterTileProvider);
    private:
        const std::unique_ptr<OnlineMapRasterTileProvider_P> _d;
    protected:
    public:
        OnlineMapRasterTileProvider(const QString& id, const QString& urlPattern,
            const ZoomLevel minZoom = MinZoomLevel, const ZoomLevel maxZoom = MaxZoomLevel,
            const uint32_t maxConcurrentDownloads = 1, const uint32_t providerTileSize = 256,
            const AlphaChannelData alphaChannelData = AlphaChannelData::Undefined);
        virtual ~OnlineMapRasterTileProvider();

        const QString id;
        const QString urlPattern;
        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;
        const uint32_t maxConcurrentDownloads;
        const uint32_t providerTileSize;
        const AlphaChannelData alphaChannelData;

        void setLocalCachePath(const QDir& localCachePath);
        const QDir& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile);
        
        static std::shared_ptr<OsmAnd::IMapBitmapTileProvider> createMapnikProvider();
        static std::shared_ptr<OsmAnd::IMapBitmapTileProvider> createCycleMapProvider();
    };

}

#endif // !defined(_OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDER_H_)