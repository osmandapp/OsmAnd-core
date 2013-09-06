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

#ifndef __MAP_STYLE_P_H_
#define __MAP_STYLE_P_H_

#include <cstdint>
#include <memory>

#include <QString>
#include <QXmlStreamReader>
#include <QHash>
#include <QMap>

#include <OsmAndCore.h>
#include <MapStyle.h>

namespace OsmAnd {

    class MapStyles_P;
    class MapStyleValueDefinition;
    class MapStyleRule;
    class MapStyleEvaluator;

    class MapStyle;
    class MapStyle_P
    {
    private:
        bool parseMetadata(QXmlStreamReader& xmlReader);
        bool parse(QXmlStreamReader& xmlReader);

        void registerBuiltinValueDefinitions();
        void registerBuiltinValueDefinition(const std::shared_ptr<const MapStyleValueDefinition>& pValueDefinition);
        std::shared_ptr<const MapStyleValueDefinition> registerValue(const MapStyleValueDefinition* pValueDefinition);
        QHash< QString, std::shared_ptr<const MapStyleValueDefinition> > _valuesDefinitions;
        uint32_t _firstNonBuiltinValueDefinitionIndex;

        bool registerRule(MapStyleRulesetType type, const std::shared_ptr<MapStyleRule>& rule);

        QHash< QString, QString > _parsetimeConstants;
        QHash< QString, std::shared_ptr<MapStyleRule> > _attributes;
    protected:
        MapStyle_P(MapStyle* owner);

        bool parseMetadata();
        bool parse();

        QString _title;

        QString _name;
        QString _parentName;
        std::shared_ptr<const MapStyle> _parent;

        bool resolveDependencies();
        bool resolveConstantValue(const QString& name, QString& value);
        QString obtainValue(const QString& value);

        bool mergeInherited();
        bool mergeInheritedRules(MapStyleRulesetType type);
        bool mergeInheritedAttributes();

        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _pointRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _lineRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _polygonRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _textRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> > _orderRules;
        QMap< uint64_t, std::shared_ptr<MapStyleRule> >& obtainRules(MapStyleRulesetType type);

        std::shared_ptr<MapStyleRule> createTagValueRootWrapperRule(uint64_t id, const std::shared_ptr<MapStyleRule>& rule);

        uint32_t _stringsIdBase;
        QList< QString > _stringsLUT;
        QHash< QString, uint32_t > _stringsRevLUT;
        uint32_t lookupStringId(const QString& value);
        uint32_t registerString(const QString& value);

        enum {
            RuleIdTagShift = 32,
        };

        MapStyle* const owner;

        static uint64_t encodeRuleId(uint32_t tag, uint32_t value);
    public:
        virtual ~MapStyle_P();

        const QMap< uint64_t, std::shared_ptr<MapStyleRule> >& obtainRules(MapStyleRulesetType type) const;

        uint32_t getTagStringId(uint64_t ruleId) const;
        uint32_t getValueStringId(uint64_t ruleId) const;
        const QString& getTagString(uint64_t ruleId) const;
        const QString& getValueString(uint64_t ruleId) const;

        bool lookupStringId(const QString& value, uint32_t& id) const;
        const QString& lookupStringValue(uint32_t id) const;

    friend class OsmAnd::MapStyle;
    friend class OsmAnd::MapStyleEvaluator;
    friend class OsmAnd::MapStyleRule;
    friend class OsmAnd::MapStyles_P;
    };

} // namespace OsmAnd

#endif // __MAP_STYLE_P_H_
