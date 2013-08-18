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

#ifndef __RASTERIZATION_STYLE_EVALUATOR_H_
#define __RASTERIZATION_STYLE_EVALUATOR_H_

#include <stdint.h>
#include <memory>

#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapStyle.h>
#include <OsmAndCore/Map/MapStyleRule.h>

namespace OsmAnd {

    namespace Model {
        class MapObject;
    }
    class MapStyleRule;

    class OSMAND_CORE_API MapStyleEvaluator
    {
    private:
    protected:
        QMap< OsmAnd::MapStyle::ValueDefinition*, OsmAnd::MapStyleRule::Value > _values;
        
        bool evaluate(uint32_t tagKey, uint32_t valueKey, bool fillOutput, bool evaluateChildren);
        bool evaluate(const std::shared_ptr<OsmAnd::MapStyleRule>& rule, bool fillOutput, bool evaluateChildren);
    public:
        MapStyleEvaluator(const std::shared_ptr<const MapStyle>& style, MapStyle::RulesetType ruleset, const std::shared_ptr<const OsmAnd::Model::MapObject>& mapObject = std::shared_ptr<const OsmAnd::Model::MapObject>());
        MapStyleEvaluator(const std::shared_ptr<const MapStyle>& style, const std::shared_ptr<MapStyleRule>& singleRule);
        virtual ~MapStyleEvaluator();

        const std::shared_ptr<const MapStyle> style;
        const std::shared_ptr<const OsmAnd::Model::MapObject> mapObject;
        const MapStyle::RulesetType ruleset;
        const std::shared_ptr<MapStyleRule> singleRule;

        void setValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const OsmAnd::MapStyleRule::Value& value);
        void setBooleanValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const bool& value);
        void setIntegerValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const int& value);
        void setIntegerValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const unsigned int& value);
        void setFloatValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const float& value);
        void setStringValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, const QString& value);

        bool getBooleanValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, bool& value) const;
        bool getIntegerValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, int& value) const;
        bool getIntegerValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, unsigned int& value) const;
        bool getFloatValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, float& value) const;
        bool getStringValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref, QString& value) const;

        void clearValue(const std::shared_ptr<OsmAnd::MapStyle::ValueDefinition>& ref);

        bool evaluate(bool fillOutput = true, bool evaluateChildren = true);

        void dump(bool input = true, bool output = true, const QString& prefix = QString()) const;
    };


} // namespace OsmAnd

#endif // __RASTERIZATION_STYLE_EVALUATOR_H_
