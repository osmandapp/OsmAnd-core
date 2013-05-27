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

#ifndef __RASTERIZATION_STYLE_H_
#define __RASTERIZATION_STYLE_H_

#include <stdint.h>
#include <memory>

#include <QString>
#include <QStringList>
#include <QFile>
#include <QXmlStreamReader>
#include <QHash>
#include <QMap>

#include <OsmAndCore.h>

namespace OsmAnd {

    class RasterizationStyles;
    class RasterizationStyle;
    class RasterizationRule;

    class OSMAND_CORE_API RasterizationStyle
    {
    public:
        enum RulesetType : uint32_t
        {
            Invalid = 0,

            Point = 1,
            Line = 2,
            Polygon = 3,
            Text = 4,
            Order = 5,
        };

        class OSMAND_CORE_API ValueDefinition
        {
        public:
            enum DataType
            {
                Boolean,
                Integer,
                Float,
                String,
                Color,
            };

            enum Type
            {
                Input,
                Output,
            };
        private:
        protected:
            ValueDefinition(ValueDefinition::Type type, ValueDefinition::DataType dataType, const QString& name);
        public:
            virtual ~ValueDefinition();

            const Type type;
            const DataType dataType;
            const QString name;

        friend class OsmAnd::RasterizationStyle;
        };

        class OSMAND_CORE_API ConfigurableInputValue : public ValueDefinition
        {
        private:
        protected:
            ConfigurableInputValue(ValueDefinition::DataType type, const QString& name, const QString& title, const QString& description, const QStringList& possibleValues);
        public:
            virtual ~ConfigurableInputValue();

            const QString title;
            const QString description;
            const QStringList possibleValues;

            friend class OsmAnd::RasterizationStyle;
        };
    private:
        bool parseMetadata(QXmlStreamReader& xmlReader);
        bool parse(QXmlStreamReader& xmlReader);

        void registerBuiltinValueDefinitions();
        void registerBuiltinValue(const std::shared_ptr<ValueDefinition>& pValueDefinition);
        std::shared_ptr<ValueDefinition> registerValue(ValueDefinition* pValueDefinition);
        QHash< QString, std::shared_ptr<ValueDefinition> > _valuesDefinitions;
        uint32_t _firstNonBuiltinValueDefinitionIndex;

        bool registerRule(RulesetType type, const std::shared_ptr<RasterizationRule>& rule);

        QHash< QString, QString > _parsetimeConstants;
        QHash< QString, std::shared_ptr<RasterizationRule> > _attributes;
    protected:
        RasterizationStyle(RasterizationStyles* owner, const QString& embeddedResourceName);
        RasterizationStyle(RasterizationStyles* owner, const QFile& externalStyleFile);

        QString _resourceName;
        QString _externalFileName;

        bool parseMetadata();
        bool parse();

        QString _title;

        QString _name;
        QString _parentName;
        std::shared_ptr<RasterizationStyle> _parent;

        bool resolveDependencies();
        bool resolveConstantValue(const QString& name, QString& value);
        QString obtainValue(const QString& value);
        QString obtainValue(const QStringRef& value);

        bool mergeInherited();
        bool mergeInheritedRules(RulesetType type);
        bool mergeInheritedAttributes();

        QMap< uint64_t, std::shared_ptr<RasterizationRule> > _pointRules;
        QMap< uint64_t, std::shared_ptr<RasterizationRule> > _lineRules;
        QMap< uint64_t, std::shared_ptr<RasterizationRule> > _polygonRules;
        QMap< uint64_t, std::shared_ptr<RasterizationRule> > _textRules;
        QMap< uint64_t, std::shared_ptr<RasterizationRule> > _orderRules;
        QMap< uint64_t, std::shared_ptr<RasterizationRule> >& obtainRules(RulesetType type);

        std::shared_ptr<RasterizationRule> createTagValueRootWrapperRule(uint64_t id, const std::shared_ptr<RasterizationRule>& rule);

        uint32_t _stringsIdBase;
        QList< QString > _stringsLUT;
        QHash< QString, uint32_t > _stringsRevLUT;
        uint32_t lookupStringId(const QString& value);
        uint32_t registerString(const QString& value);

        const QString& getTagString(uint64_t ruleId) const;
        const QString& getValueString(uint64_t ruleId) const;

        enum {
            RuleIdTagShift = 32,
        };
    public:
        virtual ~RasterizationStyle();

        RasterizationStyles* const owner;

        const QString& resourceName;
        const QString& externalFileName;

        const QString& title;

        const QString& name;
        const QString& parentName;
        
        bool isStandalone() const;
        bool areDependenciesResolved() const;

        const QMap< uint64_t, std::shared_ptr<RasterizationRule> >& obtainRules(RulesetType type) const;
        static uint64_t encodeRuleId(uint32_t tag, uint32_t value);

        bool resolveValueDefinition(const QString& name, std::shared_ptr<ValueDefinition>& outDefinition);
        bool resolveAttribute(const QString& name, std::shared_ptr<RasterizationRule>& outAttribute);

        static const struct _builtinValueDefinitions
        {
            const std::shared_ptr<ValueDefinition> INPUT_TEST;
            const std::shared_ptr<ValueDefinition> INPUT_TEXT_LENGTH;
            const std::shared_ptr<ValueDefinition> INPUT_TAG;
            const std::shared_ptr<ValueDefinition> INPUT_VALUE;
            const std::shared_ptr<ValueDefinition> INPUT_MINZOOM;
            const std::shared_ptr<ValueDefinition> INPUT_MAXZOOM;
            const std::shared_ptr<ValueDefinition> INPUT_NIGHT_MODE;
            const std::shared_ptr<ValueDefinition> INPUT_LAYER;
            const std::shared_ptr<ValueDefinition> INPUT_POINT;
            const std::shared_ptr<ValueDefinition> INPUT_AREA;
            const std::shared_ptr<ValueDefinition> INPUT_CYCLE;
            const std::shared_ptr<ValueDefinition> INPUT_NAME_TAG;
            const std::shared_ptr<ValueDefinition> INPUT_ADDITIONAL;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_SHIELD;
            const std::shared_ptr<ValueDefinition> OUTPUT_SHADOW_RADIUS;
            const std::shared_ptr<ValueDefinition> OUTPUT_SHADOW_COLOR;
            const std::shared_ptr<ValueDefinition> OUTPUT_SHADER;
            const std::shared_ptr<ValueDefinition> OUTPUT_CAP_3;
            const std::shared_ptr<ValueDefinition> OUTPUT_CAP_2;
            const std::shared_ptr<ValueDefinition> OUTPUT_CAP;
            const std::shared_ptr<ValueDefinition> OUTPUT_CAP_0;
            const std::shared_ptr<ValueDefinition> OUTPUT_CAP__1;
            const std::shared_ptr<ValueDefinition> OUTPUT_PATH_EFFECT_3;
            const std::shared_ptr<ValueDefinition> OUTPUT_PATH_EFFECT_2;
            const std::shared_ptr<ValueDefinition> OUTPUT_PATH_EFFECT;
            const std::shared_ptr<ValueDefinition> OUTPUT_PATH_EFFECT_0;
            const std::shared_ptr<ValueDefinition> OUTPUT_PATH_EFFECT__1;
            const std::shared_ptr<ValueDefinition> OUTPUT_STROKE_WIDTH_3;
            const std::shared_ptr<ValueDefinition> OUTPUT_STROKE_WIDTH_2;
            const std::shared_ptr<ValueDefinition> OUTPUT_STROKE_WIDTH;
            const std::shared_ptr<ValueDefinition> OUTPUT_STROKE_WIDTH_0;
            const std::shared_ptr<ValueDefinition> OUTPUT_STROKE_WIDTH__1;
            const std::shared_ptr<ValueDefinition> OUTPUT_COLOR_3;
            const std::shared_ptr<ValueDefinition> OUTPUT_COLOR;
            const std::shared_ptr<ValueDefinition> OUTPUT_COLOR_2;
            const std::shared_ptr<ValueDefinition> OUTPUT_COLOR_0;
            const std::shared_ptr<ValueDefinition> OUTPUT_COLOR__1;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_BOLD;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_ORDER;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_MIN_DISTANCE;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_ON_PATH;
            const std::shared_ptr<ValueDefinition> OUTPUT_ICON;
            const std::shared_ptr<ValueDefinition> OUTPUT_ORDER;
            const std::shared_ptr<ValueDefinition> OUTPUT_SHADOW_LEVEL;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_DY;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_SIZE;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_COLOR;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_HALO_RADIUS;
            const std::shared_ptr<ValueDefinition> OUTPUT_TEXT_WRAP_WIDTH;
            const std::shared_ptr<ValueDefinition> OUTPUT_OBJECT_TYPE;
            const std::shared_ptr<ValueDefinition> OUTPUT_ATTR_INT_VALUE;
            const std::shared_ptr<ValueDefinition> OUTPUT_ATTR_COLOR_VALUE;
            const std::shared_ptr<ValueDefinition> OUTPUT_ATTR_BOOL_VALUE;
            const std::shared_ptr<ValueDefinition> OUTPUT_ATTR_STRING_VALUE;

            protected:
                _builtinValueDefinitions();

            friend class OsmAnd::RasterizationStyle;
        } builtinValueDefinitions;

        bool lookupStringId(const QString& value, uint32_t& id);
        const QString& lookupStringValue(uint32_t id) const;

        void dump(const QString& prefix = QString()) const;
        void dump(RulesetType type, const QString& prefix = QString()) const;

    friend class OsmAnd::RasterizationStyles;
    friend class OsmAnd::RasterizationRule;
    };


} // namespace OsmAnd

#endif // __RASTERIZATION_STYLE_H_
