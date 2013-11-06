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

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/IMapProvider.h>

namespace OsmAnd {

    STRONG_ENUM(MapTileDataType)
    {
        Bitmap,
        ElevationData
    };

    class OSMAND_CORE_API MapTile
    {
    public:
        typedef const void* DataPtr;
    private:
    protected:
        MapTile(const MapTileDataType& dataType, const DataPtr data, size_t rowLength, uint32_t size);

        DataPtr _data;
    public:
        virtual ~MapTile();

        const MapTileDataType dataType;

        const DataPtr& data;
        const size_t rowLength;
        const uint32_t size;
    };

    class OSMAND_CORE_API IMapTileProvider : public IMapProvider
    {
    private:
    protected:
        IMapTileProvider(const MapTileDataType& dataType);
    public:
        virtual ~IMapTileProvider();

        const MapTileDataType dataType;
        virtual uint32_t getTileSize() const = 0;

        virtual bool obtainTile(const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const MapTile>& outTile) = 0;
    };

}

#endif // __I_MAP_TILE_PROVIDER_H_
