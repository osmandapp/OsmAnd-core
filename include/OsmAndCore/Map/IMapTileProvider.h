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
#ifndef __I_MAP_TILE_PROVIDER_H_
#define __I_MAP_TILE_PROVIDER_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class OSMAND_CORE_API IMapTileProvider
    {
    public:
        enum Type
        {
            Bitmap,
            ElevationData
        };

        class OSMAND_CORE_API Tile
        {
        private:
        protected:
            Tile(const Type& type, const void* data, size_t rowLength, uint32_t width, uint32_t height);
        public:
            virtual ~Tile();

            const Type type;

            const void* const data;
            const size_t rowLength;
            const uint32_t width;
            const uint32_t height;
        };

        typedef std::function<void (const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr<Tile>& tile, bool success)> TileReadyCallback;
    private:
    protected:
        IMapTileProvider(const Type& type);
    public:
        virtual ~IMapTileProvider();

        const Type type;
        virtual uint32_t getTileSize() const = 0;

        virtual void obtainTile(const TileId& tileId, const ZoomLevel& zoom, TileReadyCallback readyCallback) = 0;
    };

}

#endif // __I_MAP_TILE_PROVIDER_H_