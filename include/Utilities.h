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

#ifndef __UTILITIES_H_
#define __UTILITIES_H_

#include <OsmAndCore.h>

namespace OsmAnd {

    namespace Utilities
    {
        extern OSMAND_CORE_API const double pi;
        OSMAND_CORE_API double OSMAND_CORE_CALL toRadians(double angle);
        OSMAND_CORE_API int OSMAND_CORE_CALL get31TileNumberX(double longitude);
        OSMAND_CORE_API int OSMAND_CORE_CALL get31TileNumberY( double latitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL get31LongitudeX(int x);
        OSMAND_CORE_API double OSMAND_CORE_CALL get31LatitudeY(int y);
        OSMAND_CORE_API double OSMAND_CORE_CALL getTileNumberX(float zoom, double longitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL getTileNumberY(float zoom,  double latitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL checkLatitude(double latitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL checkLongitude(double longitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL getPowZoom(float zoom);
        OSMAND_CORE_API double OSMAND_CORE_CALL getLongitudeFromTile(float zoom, double x);
        OSMAND_CORE_API double OSMAND_CORE_CALL getLatitudeFromTile(float zoom, double y);
    } // namespace Utilities

} // namespace OsmAnd

#endif // __UTILITIES_H_