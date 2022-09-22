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
#include "IWebClient.h"

namespace OsmAnd
{
    class OnlineRasterMapLayerProvider_P Q_DECL_FINAL
    {
    private:
        static const QString buildUrlToLoad(const QString& urlToLoad, const QList<QString> randomsArray, int32_t x, int32_t y, const ZoomLevel zoom);
        static const QString eqtBingQuadKey(ZoomLevel z, int32_t x, int32_t y);
        static const QString calcBoundingBoxForTile(ZoomLevel zoom, int32_t x, int32_t y);
        const QString getUrlToLoad(int32_t x, int32_t y, const ZoomLevel zoom) const;
    protected:
        OnlineRasterMapLayerProvider_P(
            OnlineRasterMapLayerProvider* owner,
            const std::shared_ptr<const IWebClient>& downloadManager);

        const std::shared_ptr<const IWebClient> _downloadManager;

        mutable QMutex _localCachePathMutex;
        QString _localCachePath;
        bool _networkAccessAllowed;

        mutable QMutex _tilesInProcessMutex;
        std::array< QSet< TileId >, ZoomLevelsCount > _tilesInProcess;
        QWaitCondition _waitUntilAnyTileIsProcessed;

        void lockTile(const TileId tileId, const ZoomLevel zoom);
        void unlockTile(const TileId tileId, const ZoomLevel zoom);
    public:
        virtual ~OnlineRasterMapLayerProvider_P();

        ImplementationInterface<OnlineRasterMapLayerProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::OnlineRasterMapLayerProvider;
    };
}

#endif // !defined(_OSMAND_CORE_ONLINE_RASTER_MAP_TILE_PROVIDER_P_H_)
