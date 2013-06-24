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

#include <QDir>
#include <QMutex>
#include <QUrl>
#include <QQueue>
#include <QSet>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <CommonTypes.h>

#include <IMapTileProvider.h>

class QNetworkReply;

namespace OsmAnd {

    class OSMAND_CORE_API OnlineMapRasterTileProvider : public IMapTileProvider
    {
    private:
    protected:
        QString _id;
        QString _urlPattern;
        uint32_t _minZoom;
        uint32_t _maxZoom;
        uint32_t _maxConcurrentDownloads;
        QMutex _currentDownloadsMutex;
        QSet<TileId> _currentDownloads;
        
        const uint32_t _tileDimension;

        QMutex _localCacheAccessMutex;
        std::shared_ptr<QDir> _localCachePath;

        bool _networkAccessAllowed;

        QMutex _downloadQueueMutex;
        struct TileRequest
        {
            QUrl sourceUrl;
            TileId tileId;
            uint32_t zoom;
            TileReceiverCallback callback;
            SkBitmap::Config preferredConfig;
        };
        QQueue< TileRequest > _tileRequestsQueue;
        QSet< TileId > _enqueuedTileIds;

        void handleNetworkReply(QNetworkReply* reply, const TileId& tileId, uint32_t zoom, TileReceiverCallback callback, SkBitmap::Config preferredConfig);
    public:
        OnlineMapRasterTileProvider(const QString& id, const QString& urlPattern, uint32_t minZoom = 0, uint32_t maxZoom = 31, uint32_t maxConcurrentDownloads = 1, uint32_t tileDimension = 256);
        virtual ~OnlineMapRasterTileProvider();

        void setLocalCachePath(const QDir& localCachePath);
        const std::shared_ptr<QDir>& localCachePath;

        void setNetworkAccessPermission(bool allowed);
        const bool& networkAccessAllowed;

        virtual float getTileDensity() const;
        virtual uint32_t getTileSize() const;

        virtual bool obtainTileImmediate(
            const TileId& tileId, uint32_t zoom,
            std::shared_ptr<SkBitmap>& tile,
            SkBitmap::Config preferredConfig);
        virtual void obtainTileDeffered(
            const TileId& tileId, uint32_t zoom,
            TileReceiverCallback receiverCallback,
            SkBitmap::Config preferredConfig);
        void obtainTileDeffered(
            const QUrl& url,
            const TileId& tileId, uint32_t zoom,
            TileReceiverCallback receiverCallback,
            SkBitmap::Config preferredConfig);

        static std::shared_ptr<OsmAnd::IMapTileProvider> createMapnikProvider();
        static std::shared_ptr<OsmAnd::IMapTileProvider> createCycleMapProvider();
    };

}

#endif // __ONLINE_MAP_RASTER_TILE_PROVIDER_H_