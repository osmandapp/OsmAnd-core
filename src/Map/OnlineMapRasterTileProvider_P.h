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

#include <cstdint>
#include <memory>
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

#endif // __ONLINE_MAP_RASTER_TILE_PROVIDER_P_H_
