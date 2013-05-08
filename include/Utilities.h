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

#include <cstdint>

#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd {

    namespace Utilities
    {
        OSMAND_CORE_API double OSMAND_CORE_CALL toRadians(double angle);
        OSMAND_CORE_API uint32_t OSMAND_CORE_CALL get31TileNumberX(double longitude);
        OSMAND_CORE_API uint32_t OSMAND_CORE_CALL get31TileNumberY(double latitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL get31LongitudeX(uint32_t x);
        OSMAND_CORE_API double OSMAND_CORE_CALL get31LatitudeY(uint32_t y);
        OSMAND_CORE_API double OSMAND_CORE_CALL getTileNumberX(float zoom, double longitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL getTileNumberY(float zoom,  double latitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL checkLatitude(double latitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL checkLongitude(double longitude);
        OSMAND_CORE_API double OSMAND_CORE_CALL getPowZoom(float zoom);
        OSMAND_CORE_API double OSMAND_CORE_CALL getLongitudeFromTile(float zoom, double x);
        OSMAND_CORE_API double OSMAND_CORE_CALL getLatitudeFromTile(float zoom, double y);
        OSMAND_CORE_API bool OSMAND_CORE_CALL extractFirstNumberPosition(const QString& value, int& first, int& last, bool allowSigned, bool allowDot);
        OSMAND_CORE_API double OSMAND_CORE_CALL parseSpeed(const QString& value, double defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API double OSMAND_CORE_CALL parseLength(const QString& value, double defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API double OSMAND_CORE_CALL parseWeight(const QString& value, double defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API int OSMAND_CORE_CALL parseArbitraryInt(const QString& value, int defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API long OSMAND_CORE_CALL parseArbitraryLong(const QString& value, long defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API unsigned int OSMAND_CORE_CALL parseArbitraryUInt(const QString& value, unsigned int defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API unsigned long OSMAND_CORE_CALL parseArbitraryULong(const QString& value, unsigned long defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API float OSMAND_CORE_CALL parseArbitraryFloat(const QString& value, float defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API bool OSMAND_CORE_CALL parseArbitraryBool(const QString& value, bool defValue, bool* wasParsed = nullptr);
        OSMAND_CORE_API double OSMAND_CORE_CALL x31toMeters(int64_t x31);
        OSMAND_CORE_API double OSMAND_CORE_CALL y31toMeters(int64_t y31);
        OSMAND_CORE_API double OSMAND_CORE_CALL squareDistance31(uint32_t x31a, uint32_t y31a, uint32_t x31b, uint32_t y31b);
        OSMAND_CORE_API double OSMAND_CORE_CALL distance31(uint32_t x31a, uint32_t y31a, uint32_t x31b, uint32_t y31b);
        OSMAND_CORE_API double OSMAND_CORE_CALL distance(double xLonA, double yLatA, double xLonB, double yLatB);
        OSMAND_CORE_API double OSMAND_CORE_CALL projection31(uint32_t x31a, uint32_t y31a, uint32_t x31b, uint32_t y31b, uint32_t x31c, uint32_t y31c);
        OSMAND_CORE_API double OSMAND_CORE_CALL normalizedAngleRadians(double angle);
        OSMAND_CORE_API double OSMAND_CORE_CALL normalizedAngleDegrees(double angle);
        OSMAND_CORE_API int OSMAND_CORE_CALL javaDoubleCompare(double l, double r);
    } // namespace Utilities

} // namespace OsmAnd

#endif // __UTILITIES_H_