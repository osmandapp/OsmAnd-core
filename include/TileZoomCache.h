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
#ifndef __TILE_ZOOM_CACHE_H_
#define __TILE_ZOOM_CACHE_H_

#include <stdint.h>
#include <memory>
#include <array>

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class OSMAND_CORE_API TileZoomCache
    {
    public:
        struct OSMAND_CORE_API Tile
        {
            Tile(const uint32_t& zoom, const TileId& id, const size_t& usedMemory);
            virtual ~Tile();

            const uint32_t zoom;
            const TileId id;
            const size_t usedMemory;
        };
    private:
    protected:
        struct OSMAND_CORE_API ZoomLevel
        {
            ZoomLevel();
            virtual ~ZoomLevel();

            QHash< TileId, std::shared_ptr<Tile> > _tiles;
            size_t _usedMemory;
        };
        virtual std::shared_ptr<ZoomLevel> allocateCachedZoomLevel(uint32_t zoom);
        std::array< std::shared_ptr<ZoomLevel>, 32 > _zoomLevels;

        uint64_t _tilesCount;
        size_t _usedMemory;
    public:
        TileZoomCache();
        virtual ~TileZoomCache();

        const uint64_t& tilesCount;
        const size_t& usedMemory;

        void putTile(const std::shared_ptr<Tile>& tile);
        bool getTile(const uint32_t& zoom, const TileId& id, std::shared_ptr<Tile>& tileOut) const;
        bool contains(const uint32_t& zoom, const TileId& id) const;

        void clearAll();

        void clearExceptCube(const AreaI& bbox31, uint32_t fromZoom, uint32_t toZoom, bool byDistanceOrder = false, size_t untilMemoryThreshold = 0);
        void clearExceptInterestBox(const AreaI& bbox31, uint32_t baseZoom, uint32_t fromZoom, uint32_t toZoom, float interestFactor = 2.0f, bool byDistanceOrder = false, size_t untilMemoryThreshold = 0);
        void clearExceptInterestSet(const QSet<TileId>& tiles, uint32_t baseZoom, uint32_t fromZoom, uint32_t toZoom, float interestFactor = 2.0f, bool byDistanceOrder = false, size_t untilMemoryThreshold = 0);
    };

}

#endif // __TILE_ZOOM_CACHE_H_
