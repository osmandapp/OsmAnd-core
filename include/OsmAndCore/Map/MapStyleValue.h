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

#ifndef _OSMAND_CORE_MAP_STYLE_VALUE_H_
#define _OSMAND_CORE_MAP_STYLE_VALUE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>

namespace OsmAnd {

    class MapStyles;
    class MapStyles_P;
    class MapStyleEvaluator;
    class MapStyleEvaluator_P;

    class MapStyleValueDefinition;
    class MapStyleRule;

    struct OSMAND_CORE_API MapStyleValue
    {
        MapStyleValue();

        bool isComplex;

        template<typename T> struct ComplexData
        {
            T dip;
            T px;

            inline T evaluate(const float densityFactor) const
            {
                return dip*densityFactor + px;
            }
        };

        union {
            union {
                float asFloat;
                int32_t asInt;
                uint32_t asUInt;

                double asDouble;
                int64_t asInt64;
                uint64_t asUInt64;
            } asSimple;

            union {
                ComplexData<float> asFloat;
                ComplexData<int32_t> asInt;
                ComplexData<uint32_t> asUInt;
            } asComplex;
        };
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_MAP_STYLE_VALUE_H_
