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

#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyle.h>

namespace OsmAnd {

    namespace Model {
        class MapObject;
    } // namespace Model
    class MapStyleRule;
    class MapStyleValueDefinition;

    class OSMAND_CORE_API MapStyleEvaluator
    {
    private:
    protected:
        QMap< std::shared_ptr<const OsmAnd::MapStyleValueDefinition>, OsmAnd::MapStyleValue > _values;
        
        bool evaluate(uint32_t tagKey, uint32_t valueKey, bool fillOutput, bool evaluateChildren);
        bool evaluate(const std::shared_ptr<const MapStyleRule>& rule, bool fillOutput, bool evaluateChildren);
    public:
        MapStyleEvaluator(const std::shared_ptr<const MapStyle>& style, MapStyleRulesetType ruleset, const std::shared_ptr<const OsmAnd::Model::MapObject>& mapObject = std::shared_ptr<const OsmAnd::Model::MapObject>());
        MapStyleEvaluator(const std::shared_ptr<const MapStyle>& style, const std::shared_ptr<const MapStyleRule>& singleRule);
        virtual ~MapStyleEvaluator();

        const std::shared_ptr<const MapStyle> style;
        const std::shared_ptr<const OsmAnd::Model::MapObject> mapObject;
        const MapStyleRulesetType ruleset;
        const std::shared_ptr<const MapStyleRule> singleRule;

        void setValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, const OsmAnd::MapStyleValue& value);
        void setBooleanValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, const bool& value);
        void setIntegerValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, const int& value);
        void setIntegerValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, const unsigned int& value);
        void setFloatValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, const float& value);
        void setStringValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, const QString& value);

        bool getBooleanValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, bool& value) const;
        bool getIntegerValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, int& value) const;
        bool getIntegerValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, unsigned int& value) const;
        bool getFloatValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, float& value) const;
        bool getStringValue(const std::shared_ptr<const MapStyleValueDefinition>& ref, QString& value) const;

        void clearValue(const std::shared_ptr<const MapStyleValueDefinition>& ref);

        bool evaluate(bool fillOutput = true, bool evaluateChildren = true);

        void dump(bool input = true, bool output = true, const QString& prefix = QString()) const;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_EVALUATOR_H_
