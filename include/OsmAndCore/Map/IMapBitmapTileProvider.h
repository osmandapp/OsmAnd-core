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
#ifndef _OSMAND_CORE_I_MAP_BITMAP_TILE_PROVIDER_H_
#define _OSMAND_CORE_I_MAP_BITMAP_TILE_PROVIDER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapTileProvider.h>

class SkBitmap;

namespace OsmAnd {

    class OSMAND_CORE_API MapBitmapTile : public MapTile
    {
        Q_DISABLE_COPY(MapBitmapTile);
    public:
        STRONG_ENUM(AlphaChannelData)
        {
            Present,
            NotPresent,
            Undefined
        };

    private:
    protected:
        std::unique_ptr<SkBitmap> _bitmap;
    public:
        MapBitmapTile(SkBitmap* bitmap, const AlphaChannelData& alphaChannelData);
        virtual ~MapBitmapTile();

        const std::unique_ptr<SkBitmap>& bitmap;

        const AlphaChannelData alphaChannelData;
    };

    class OSMAND_CORE_API IMapBitmapTileProvider : public IMapTileProvider
    {
    public:
    private:
    protected:
        IMapBitmapTileProvider();
    public:
        virtual ~IMapBitmapTileProvider();

        virtual float getTileDensity() const = 0;
    };
}

#endif // _OSMAND_CORE_I_MAP_BITMAP_TILE_PROVIDER_H_
