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
#ifndef __I_MAP_BITMAP_TILE_PROVIDER_H_
#define __I_MAP_BITMAP_TILE_PROVIDER_H_

#include <stdint.h>
#include <memory>
#include <functional>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTileProvider.h>

class SkBitmap;

namespace OsmAnd {

    class OSMAND_CORE_API IMapBitmapTileProvider : public IMapTileProvider
    {
    public:
        enum class AlphaChannelData
        {
            Present,
            NotPresent,
            Undefined
        };

        class OSMAND_CORE_API Tile : public IMapTileProvider::Tile
        {
        private:
            std::unique_ptr<SkBitmap> _bitmap;
        protected:
        public:
            Tile(SkBitmap* bitmap, const AlphaChannelData& alphaChannelData);
            virtual ~Tile();

            const std::unique_ptr<SkBitmap>& bitmap;

            const AlphaChannelData alphaChannelData;
        };
    private:
    protected:
        IMapBitmapTileProvider();
    public:
        virtual ~IMapBitmapTileProvider();

        virtual float getTileDensity() const = 0;
    };
}

#endif // __I_MAP_BITMAP_TILE_PROVIDER_H_
