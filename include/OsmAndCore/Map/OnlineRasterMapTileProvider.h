#ifndef _OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QDir>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/IMapRasterBitmapTileProvider.h>

namespace OsmAnd
{
    class OnlineRasterMapTileProvider_P;
    class OSMAND_CORE_API OnlineRasterMapTileProvider : public IMapRasterBitmapTileProvider
    {
        Q_DISABLE_COPY_AND_MOVE(OnlineRasterMapTileProvider);
    private:
        PrivateImplementation<OnlineRasterMapTileProvider_P> _p;
    protected:
    public:
        OnlineRasterMapTileProvider(const QString& name, const QString& urlPattern,
            const ZoomLevel minZoom = MinZoomLevel, const ZoomLevel maxZoom = MaxZoomLevel,
            const uint32_t maxConcurrentDownloads = 1, const uint32_t providerTileSize = 256,
            const AlphaChannelData alphaChannelData = AlphaChannelData::Undefined);
        virtual ~OnlineRasterMapTileProvider();

        const QString name;
        const QString pathSuffix;
        const QString urlPattern;
#if !defined(SWIG)
        //NOTE: This stuff breaks swig due to conflict with get*();
        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;
#endif // !defined(SWIG)
        const uint32_t maxConcurrentDownloads;
        const uint32_t providerTileSize;
        const AlphaChannelData alphaChannelData;

        void setLocalCachePath(const QDir& localCachePath, const bool appendPathSuffix = true);
        const QDir& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual float getTileDensityFactor() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<MapTiledData>& outTiledData,
            const IQueryController* const queryController = nullptr);

        virtual ZoomLevel getMinZoom() const;
        virtual ZoomLevel getMaxZoom() const;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_H_)