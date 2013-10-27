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
#ifndef __I_MAP_SYMBOL_PROVIDER_H_
#define __I_MAP_SYMBOL_PROVIDER_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

class SkBitmap;

namespace OsmAnd {

    class OSMAND_CORE_API MapSymbol
    {
        Q_DISABLE_COPY(MapSymbol);
    private:
    protected:
        MapSymbol(const uint64_t id, const PointI& location, const ZoomLevel minZoom, const ZoomLevel maxZoom, SkBitmap* bitmap);

        std::unique_ptr<SkBitmap> _bitmap;
    public:
        virtual ~MapSymbol();

        const uint64_t id;

        const PointI location;
        const ZoomLevel minZoom;
        const ZoomLevel maxZoom;

        const std::unique_ptr<SkBitmap>& bitmap;
    };

    class OSMAND_CORE_API IMapSymbolProvider
    {
    private:
    protected:
        IMapSymbolProvider();
    public:
        virtual ~IMapSymbolProvider();

        virtual bool obtainSymbols(const TileId tileId, const ZoomLevel zoom, QList< std::shared_ptr<const MapSymbol> >& outSymbols) = 0;
    };

}

#endif // __I_MAP_SYMBOL_PROVIDER_H_
