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
#ifndef __OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_P_H_
#define __OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_P_H_

#include <cstdint>
#include <memory>
#include <functional>
#include <array>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <Concurrent.h>
#include <TilesCollection.h>
#include <IMapBitmapTileProvider.h>

namespace OsmAnd {

    class OfflineMapRasterTileProvider_GPU;
    class OfflineMapRasterTileProvider_GPU_P
    {
    private:
    protected:
        OfflineMapRasterTileProvider_GPU_P(OfflineMapRasterTileProvider_GPU* owner, const uint32_t& outputTileSize, const float& density);

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

        OfflineMapRasterTileProvider_GPU* const owner;
        const uint32_t outputTileSize;
        const float density;

        const Concurrent::TaskHost::Bridge _taskHostBridge;
        TilesCollection<TileEntry> _tiles;

        bool obtainTile(const TileId& tileId, const ZoomLevel& zoom, std::shared_ptr<MapTile>& outTile);
    public:
        virtual ~OfflineMapRasterTileProvider_GPU_P();

    friend class OsmAnd::OfflineMapRasterTileProvider_GPU;
    };

}

#endif // __OFFLINE_MAP_RASTER_TILE_PROVIDER_GPU_P_H_
