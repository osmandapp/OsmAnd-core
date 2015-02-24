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
#include "IRasterMapLayerProvider.h"
#include "OnlineRasterMapLayerProvider.h"
#include "WebClient.h"

namespace OsmAnd
{
    class OnlineRasterMapLayerProvider_P Q_DECL_FINAL
    {
    private:
    protected:
        OnlineRasterMapLayerProvider_P(OnlineRasterMapLayerProvider* owner);

        mutable QMutex _localCachePathMutex;
        QString _localCachePath;
        bool _networkAccessAllowed;

        mutable QMutex _tilesInProcessMutex;
        std::array< QSet< TileId >, ZoomLevelsCount > _tilesInProcess;
        QWaitCondition _waitUntilAnyTileIsProcessed;

        WebClient _downloadManager;

        void lockTile(const TileId tileId, const ZoomLevel zoom);
        void unlockTile(const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~OnlineRasterMapLayerProvider_P();

        ImplementationInterface<OnlineRasterMapLayerProvider> owner;

        bool obtainData(
            const TileId tileId,
            const ZoomLevel zoom,
            std::shared_ptr<OnlineRasterMapLayerProvider::Data>& outTiledData,
            const IQueryController* const queryController);

    friend class OsmAnd::OnlineRasterMapLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_P_H_)
