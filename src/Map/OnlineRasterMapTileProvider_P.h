#ifndef _OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_P_H_
#define _OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_P_H_

#include "stdlib_common.h"
#include <array>

#include "QtExtensions.h"
#include <QSet>
#include <QDir>
#include <QUrl>
#include <QNetworkReply>
#include <QEventLoop>
#include <QMutex>
#include <QWaitCondition>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapRasterBitmapTileProvider.h"
#include "WebClient.h"

namespace OsmAnd
{
    class OnlineRasterMapTileProvider;
    class OnlineRasterMapTileProvider_P
    {
    private:
    protected:
        OnlineRasterMapTileProvider_P(OnlineRasterMapTileProvider* owner);

        mutable QMutex _localCachePathMutex;
        QDir _localCachePath;
        bool _networkAccessAllowed;

        mutable QMutex _tilesInProcessMutex;
        std::array< QSet< TileId >, ZoomLevelsCount > _tilesInProcess;
        QWaitCondition _waitUntilAnyTileIsProcessed;

        WebClient _downloadManager;

        bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile, const IQueryController* const queryController);
        void lockTile(const TileId tileId, const ZoomLevel zoom);
        void unlockTile(const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~OnlineRasterMapTileProvider_P();

        ImplementationInterface<OnlineRasterMapTileProvider> owner;

    friend class OsmAnd::OnlineRasterMapTileProvider;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_P_H_)
