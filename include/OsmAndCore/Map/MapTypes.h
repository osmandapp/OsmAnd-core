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

#if !defined(__MAP_TYPES_H_)
#define __MAP_TYPES_H_

#include <stdint.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    WEAK_ENUM_EX(MapTileLayerId, int32_t)
    {
        Invalid = -1,

        ElevationData,
        RasterMap,
        MapOverlay0,
        MapOverlay1,
        MapOverlay2,
        MapOverlay3,

        __LAST,
    };
    enum {
        MapTileLayerIdsCount = static_cast<unsigned>(MapTileLayerId::__LAST)
    };
}

#endif // __MAP_TYPES_H_
