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
#ifndef __ONLINE_MAP_RASTER_TILE_PROVIDER_P_H_
#define __ONLINE_MAP_RASTER_TILE_PROVIDER_P_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QDir>
#include <QUrl>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <TilesCollection.h>
#include <Concurrent.h>
#include <IMapTileProvider.h>
#include <IMapTileProvider.h>

namespace OsmAnd {

    class OnlineMapRasterTileProvider;
    class OnlineMapRasterTileProvider_P
    {
    private:
    protected:
        OnlineMapRasterTileProvider_P(OnlineMapRasterTileProvider* owner);

        STRONG_ENUM(TileState)
        {
            // Tile is not in any determined state (tile entry did not exist)
            Unknown = 0,

            // Tile is requested
            Requested,

            // Tile is being searched locally
            LocalLookup,

            // Tile is enqueued for download
            EnqueuedForDownload,

            // Tile is being downloaded
            Downloading,
        };

        class TileEntry : public TilesCollectionEntryWithState<TileState, TileState::Unknown>
        {
        private:
        protected:
            QUrl sourceUrl;
            QFileInfo localPath;
            IMapTileProvider::TileReadyCallback callback;
        public:
            TileEntry(const TileId& tileId, const ZoomLevel& zoom);
            virtual ~TileEntry();

            friend class OsmAnd::OnlineMapRasterTileProvider;
            friend class OsmAnd::OnlineMapRasterTileProvider_P;
        };

        const OnlineMapRasterTileProvider* owner;
        const Concurrent::TaskHost::Bridge _taskHostBridge;

        QMutex _currentDownloadsCounterMutex;
        uint32_t _currentDownloadsCount;
        QDir _localCachePath;
        bool _networkAccessAllowed;

        void obtainTileDeffered(const TileId& tileId, const ZoomLevel& zoom, IMapTileProvider::TileReadyCallback readyCallback);
        void obtainTileDeffered(const std::shared_ptr<TileEntry>& tileEntry);
        void replyFinishedHandler(QNetworkReply* reply, const std::shared_ptr<TileEntry>& tileEntry, QEventLoop& eventLoop, QNetworkAccessManager& networkAccessManager);
        void handleNetworkReply(QNetworkReply* reply, const std::shared_ptr<TileEntry>& tileEntry);

        TilesCollection<TileEntry> _tiles;
    public:
        virtual ~OnlineMapRasterTileProvider_P();

    friend class OsmAnd::OnlineMapRasterTileProvider;
    };

}

#endif // __ONLINE_MAP_RASTER_TILE_PROVIDER_P_H_
