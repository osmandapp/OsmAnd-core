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
    private:
    protected:
        IMapTileProvider();
    public:
        virtual ~IMapTileProvider();

        virtual bool obtainTile(
            const uint64_t& tileId, uint32_t zoom,
            std::shared_ptr<SkBitmap>& tile) = 0;
        virtual void obtainTile(
            const uint64_t& tileId, uint32_t zoom,
            std::function<void (const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tile)> receiverCallback) = 0;
    };

}

#endif // __I_MAP_TILE_PROVIDER_H_