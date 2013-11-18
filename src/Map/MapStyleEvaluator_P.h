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

#ifndef __MAP_STYLE_EVALUATOR_P_H_
#define __MAP_STYLE_EVALUATOR_P_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>

#include <OsmAndCore.h>
#include <MapStyle.h>

namespace OsmAnd {

    class MapStyleEvaluationResult;
    class MapStyleBuiltinValueDefinitions;
    
    class MapStyleEvaluator;
    class MapStyleEvaluator_P
    {
    public:
        union InputValue
        {
            inline InputValue()
            {
                asUInt = 0;
            }

            float asFloat;
            int32_t asInt;
            uint32_t asUInt;
        };
    private:
    protected:
        MapStyleEvaluator_P(MapStyleEvaluator* owner);

        MapStyleEvaluator* const owner;

        const std::shared_ptr<const MapStyleBuiltinValueDefinitions> _builtinValueDefs;

        QMap<int, InputValue> _inputValues;

        bool evaluate(const uint32_t tagKey, const uint32_t valueKey, MapStyleEvaluationResult* const outResultStorage, bool evaluateChildren);
        bool evaluate(const std::shared_ptr<const MapStyleRule>& rule, MapStyleEvaluationResult* const outResultStorage, bool evaluateChildren);
        bool evaluate(MapStyleEvaluationResult* const outResultStorage, bool evaluateChildren);
    public:
        ~MapStyleEvaluator_P();

    friend class OsmAnd::MapStyleEvaluator;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_EVALUATOR_P_H_
