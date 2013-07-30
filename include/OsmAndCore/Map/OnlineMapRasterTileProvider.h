/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ONLINE_MAP_RASTER_TILE_PROVIDER_H_
#define __ONLINE_MAP_RASTER_TILE_PROVIDER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QDir>
#include <QMutex>
#include <QUrl>
#include <QQueue>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

#include <OsmAndCore/Map/IMapBitmapTileProvider.h>

class QNetworkAccessManager;
class QEventLoop;
class QNetworkReply;

namespace OsmAnd {

    class OSMAND_CORE_API OnlineMapRasterTileProvider : public IMapBitmapTileProvider
    {
    private:
    protected:
        QMutex _processingMutex;
        struct TileRequest
        {
            QUrl sourceUrl;
            TileId tileId;
            ZoomLevel zoom;
            TileReadyCallback callback;
        };
        QQueue< TileRequest > _tileDownloadRequestsQueue;
        std::array< QSet< TileId >, 32 > _enqueuedTileIdsForDownload;
        std::array< QSet< TileId >, 32 > _currentlyDownloadingTileIds;
        uint32_t _currentDownloadsCount;
        std::shared_ptr<QDir> _localCachePath;
        bool _networkAccessAllowed;

        QMutex _requestsMutex;
        std::array< QSet< TileId >, 32 > _requestedTileIds;

        void obtainTileDeffered(const QUrl& url, const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback);
        void replyFinishedHandler(QNetworkReply* reply, const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback, QEventLoop& eventLoop, QNetworkAccessManager& networkAccessManager);
        void handleNetworkReply(QNetworkReply* reply, const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback);
    public:
        OnlineMapRasterTileProvider(const QString& id, const QString& urlPattern, const ZoomLevel& minZoom = ZoomLevel0, const ZoomLevel& maxZoom = ZoomLevel31, uint32_t maxConcurrentDownloads = 1, uint32_t tileDimension = 256);
        virtual ~OnlineMapRasterTileProvider();

        const QString id;
        const QString urlPattern;
        const uint32_t minZoom;
        const uint32_t maxZoom;
        const uint32_t maxConcurrentDownloads;
        const uint32_t tileDimension;

        void setLocalCachePath(const QDir& localCachePath);
        const std::shared_ptr<QDir>& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTileImmediate(const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<IMapTileProvider::Tile>& tile);
        virtual void obtainTileDeffered(const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback);
        
        static std::shared_ptr<OsmAnd::IMapTileProvider> createMapnikProvider();
        static std::shared_ptr<OsmAnd::IMapTileProvider> createCycleMapProvider();
    };

}

#endif // __ONLINE_MAP_RASTER_TILE_PROVIDER_H_