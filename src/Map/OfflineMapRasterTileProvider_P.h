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
#ifndef __OFFLINE_MAP_RASTER_TILE_PROVIDER_P_H_
#define __OFFLINE_MAP_RASTER_TILE_PROVIDER_P_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <Concurrent.h>
#include <TilesCollection.h>
#include <IMapTileProvider.h>

namespace OsmAnd {

    class OfflineMapRasterTileProvider;
    class OfflineMapRasterTileProvider_P
    {
    private:
    protected:
        OfflineMapRasterTileProvider_P(OfflineMapRasterTileProvider* owner);

        STRONG_ENUM(TileState)
        {
            // Tile is not in any determined state (tile entry did not exist)
            Unknown = 0,

            // Tile is requested
            Requested,

            // Tile is processing
            Processing,
        };
        typedef TilesCollectionEntryWithState<TileState, TileState::Unknown> TileEntry;

        OfflineMapRasterTileProvider* const owner; 

        const Concurrent::TaskHost::Bridge _taskHostBridge;
        TilesCollection<TileEntry> _tiles;

        void obtainTile(const TileId& tileId, const ZoomLevel& zoom, IMapTileProvider::TileReadyCallback readyCallback);
    public:
        virtual ~OfflineMapRasterTileProvider_P();

    friend class OsmAnd::OfflineMapRasterTileProvider;
    };

}

#endif // __OFFLINE_MAP_RASTER_TILE_PROVIDER_P_H_
