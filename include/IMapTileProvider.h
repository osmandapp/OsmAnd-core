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

#include <stdint.h>
#include <memory>
#include <functional>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class OSMAND_CORE_API IMapTileProvider
    {
    public:
        typedef std::function<void (const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tile)> TileReceiverCallback;
    private:
    protected:
        IMapTileProvider();
    public:
        virtual ~IMapTileProvider();

        virtual float getTileDensity() const = 0;
        virtual uint32_t getTileSize() const = 0;

        virtual bool obtainTile(
            const TileId& tileId, uint32_t zoom,
            std::shared_ptr<SkBitmap>& tile,
            SkBitmap::Config preferredConfig) = 0;
        virtual void obtainTile(
            const TileId& tileId, uint32_t zoom,
            TileReceiverCallback receiverCallback,
            SkBitmap::Config preferredConfig) = 0;
    };

}

#endif // __I_MAP_TILE_PROVIDER_H_