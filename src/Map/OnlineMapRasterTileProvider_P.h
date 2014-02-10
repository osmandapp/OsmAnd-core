#ifndef _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDER_P_H_
#define _OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDER_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>
#include <QDir>
#include <QUrl>
#include <QNetworkReply>
#include <QEventLoop>
#include <QMutex>
#include <QWaitCondition>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <IMapBitmapTileProvider.h>

namespace OsmAnd {

    class OnlineMapRasterTileProvider;
    class OnlineMapRasterTileProvider_P
    {
    private:
    protected:
        OnlineMapRasterTileProvider_P(OnlineMapRasterTileProvider* owner);

        const OnlineMapRasterTileProvider* owner;

        mutable QMutex _currentDownloadsCounterMutex;
        uint32_t _currentDownloadsCounter;
        QWaitCondition _currentDownloadsCounterChanged;

        mutable QMutex _localCachePathMutex;
        QDir _localCachePath;
        bool _networkAccessAllowed;

        mutable QMutex _tilesInProcessMutex;
        std::array< QSet< TileId >, ZoomLevelsCount > _tilesInProcess;
        QWaitCondition _waitUntilAnyTileIsProcessed;

        bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile);
        void lockTile(const TileId tileId, const ZoomLevel zoom);
        void unlockTile(const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~OnlineMapRasterTileProvider_P();

    friend class OsmAnd::OnlineMapRasterTileProvider;
    };

}

#endif // !defined(_OSMAND_CORE_ONLINE_MAP_RASTER_TILE_PROVIDER_P_H_)
