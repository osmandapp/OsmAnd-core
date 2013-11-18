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

#ifndef __MAP_STYLE_EVALUATOR_H_
#define __MAP_STYLE_EVALUATOR_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyle.h>

namespace OsmAnd {

    namespace Model {
        class MapObject;
    } // namespace Model
    class MapStyleRule;
    class MapStyleValueDefinition;
    struct MapStyleValue;
    class MapStyleEvaluationResult;

    class MapStyleEvaluator_P;
    class OSMAND_CORE_API MapStyleEvaluator
    {
        Q_DISABLE_COPY(MapStyleEvaluator);
    private:
        const std::unique_ptr<MapStyleEvaluator_P> _d;
    protected:
    public:
        MapStyleEvaluator(const std::shared_ptr<const MapStyle>& style, const float displayDensityFactor);
        virtual ~MapStyleEvaluator();

        const std::shared_ptr<const MapStyle> style;
        const float displayDensityFactor;

        void setBooleanValue(const int valueDefId, const bool value);
        void setIntegerValue(const int valueDefId, const int value);
        void setIntegerValue(const int valueDefId, const unsigned int value);
        void setFloatValue(const int valueDefId, const float value);
        void setStringValue(const int valueDefId, const QString& value);

        bool evaluate(const std::shared_ptr<const Model::MapObject>& mapObject, const MapStyleRulesetType ruleset, MapStyleEvaluationResult* const outResultStorage = nullptr, bool evaluateChildren = true);
        bool evaluate(const std::shared_ptr<const MapStyleRule>& singleRule, MapStyleEvaluationResult* const outResultStorage = nullptr, bool evaluateChildren = true);

        void dump(bool input = true, bool output = true, const QString& prefix = QString()) const;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_EVALUATOR_H_
