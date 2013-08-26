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

#ifndef __MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_
#define __MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_

#include <stdint.h>
#include <memory>

#include <OsmAndCore.h>

namespace OsmAnd {

    class MapStyleValueDefinition;
    class MapStyle;

    class OSMAND_CORE_API MapStyleBuiltinValueDefinitions
    {
    private:
    protected:
        MapStyleBuiltinValueDefinitions();
    public:
        virtual ~MapStyleBuiltinValueDefinitions();

        const std::shared_ptr<const MapStyleValueDefinition> INPUT_TEST;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_TEXT_LENGTH;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_TAG;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_VALUE;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_MINZOOM;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_MAXZOOM;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_NIGHT_MODE;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_LAYER;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_POINT;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_AREA;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_CYCLE;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_NAME_TAG;
        const std::shared_ptr<const MapStyleValueDefinition> INPUT_ADDITIONAL;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_DISABLE;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_NAME_TAG2;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_SHIELD;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_SHADOW_RADIUS;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_SHADOW_COLOR;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_SHADER;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_CAP_3;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_CAP_2;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_CAP;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_CAP_0;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_CAP__1;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_PATH_EFFECT_3;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_PATH_EFFECT_2;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_PATH_EFFECT;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_PATH_EFFECT_0;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_PATH_EFFECT__1;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_STROKE_WIDTH_3;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_STROKE_WIDTH_2;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_STROKE_WIDTH;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_STROKE_WIDTH_0;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_STROKE_WIDTH__1;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_COLOR_3;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_COLOR;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_COLOR_2;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_COLOR_0;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_COLOR__1;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_BOLD;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_ORDER;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_MIN_DISTANCE;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_ON_PATH;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_ICON;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_ICON_ORDER;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_ORDER;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_SHADOW_LEVEL;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_DY;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_SIZE;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_COLOR;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_HALO_RADIUS;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_TEXT_WRAP_WIDTH;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_OBJECT_TYPE;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_ATTR_INT_VALUE;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_ATTR_COLOR_VALUE;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_ATTR_BOOL_VALUE;
        const std::shared_ptr<const MapStyleValueDefinition> OUTPUT_ATTR_STRING_VALUE;

    friend class OsmAnd::MapStyle;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_BUILTIN_VALUE_DEFINITIONS_H_
