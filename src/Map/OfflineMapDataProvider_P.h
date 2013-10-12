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

#ifndef __OFFLINE_MAP_DATA_PROVIDER_P_H_
#define __OFFLINE_MAP_DATA_PROVIDER_P_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <TilesCollection.h>

namespace OsmAnd {

    class OfflineMapDataProvider;
    class OfflineMapDataProvider_P
    {
    public:
        STRONG_ENUM(TileState)
        {
            // Tile is not in any determined state (tile entry did not exist)
            Unknown = 0,

            // Resource was requested and should arrive soon
            Requested,

            // Resource data is in main memory, but not yet uploaded into GPU
            Ready,
        };

        class Tile : public TilesCollectionEntryWithState<TileState, TileState::Unknown>
        {
        private:
        protected:
            Tile();
        public:
            ~Tile();

        friend class OsmAnd::OfflineMapDataProvider_P;
        };
    private:
    protected:
        OfflineMapDataProvider_P(OfflineMapDataProvider* owner);

        OfflineMapDataProvider* const owner;

//        TilesCollection<Tile>;
    public:
        ~OfflineMapDataProvider_P();

        bool obtainMapPrimitivesTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<Tile>& outTile);
        //Tile data consists of rasterizer preprocessed primitives?
        //void obtainTileData(TileId, zoom, callback);
        //void purgeCache();
        //void purgeCacheExcept(visibleUniqueTiles);

    friend class OsmAnd::OfflineMapDataProvider;
    };

} // namespace OsmAnd

#endif // __OFFLINE_MAP_DATA_PROVIDER_P_H_
