#ifndef _OSMAND_CORE_RESOLVED_MAP_STYLE_H_
#define _OSMAND_CORE_RESOLVED_MAP_STYLE_H_

#include <OsmAndCore/stdlib_common.h>
#include <array>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Map/UnresolvedMapStyle.h>
#include <OsmAndCore/Map/MapStyleConstantValue.h>
#include <OsmAndCore/Map/MapStyleValueDefinition.h>
#include <OsmAndCore/Map/IMapStyle.h>

namespace OsmAnd
{
    class ResolvedMapStyle_P;
    class OSMAND_CORE_API ResolvedMapStyle : public IMapStyle
    {
        Q_DISABLE_COPY_AND_MOVE(ResolvedMapStyle);
    public:
        typedef unsigned int StringId;
        enum : unsigned int {
            EmptyStringId = 0u
        };

        typedef int ValueDefinitionId;
        
        class Attribute;

        struct OSMAND_CORE_API ResolvedValue Q_DECL_FINAL
        {
            ResolvedValue();
            ~ResolvedValue();

            bool isDynamic;
            
            MapStyleConstantValue asConstantValue;
            struct {
                std::shared_ptr<const Attribute> attribute;
            } asDynamicValue;

            static ResolvedValue fromConstantValue(const MapStyleConstantValue& input);
            static ResolvedValue fromAttribute(const std::shared_ptr<const Attribute>& attribute);
        };

        class OSMAND_CORE_API RuleNode Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(RuleNode);

        private:
        protected:
        public:
            RuleNode(const bool isSwitch);
            ~RuleNode();

            const bool isSwitch;

            QHash<ValueDefinitionId, ResolvedValue> values;
            QList< std::shared_ptr<SWIG_CLARIFY(ResolvedMapStyle, RuleNode)> > oneOfConditionalSubnodes;
            QList< std::shared_ptr<SWIG_CLARIFY(ResolvedMapStyle, RuleNode)> > applySubnodes;
        };

        class OSMAND_CORE_API BaseRule
        {
            Q_DISABLE_COPY_AND_MOVE(BaseRule);

        private:
        protected:
            BaseRule(RuleNode* const ruleNode);
        public:
            virtual ~BaseRule();

            const std::shared_ptr<RuleNode> rootNode;
        };

        class OSMAND_CORE_API Rule Q_DECL_FINAL : public BaseRule
        {
            Q_DISABLE_COPY_AND_MOVE(Rule);

        private:
        protected:
        public:
            Rule(const MapStyleRulesetType rulesetType);
            virtual ~Rule();

            const MapStyleRulesetType rulesetType;
        };

        class OSMAND_CORE_API Attribute Q_DECL_FINAL : public BaseRule
        {
            Q_DISABLE_COPY_AND_MOVE(Attribute);

        private:
        protected:
        public:
            Attribute(const StringId nameId);
            virtual ~Attribute();

            const StringId nameId;
        };
        
        class OSMAND_CORE_API Parameter Q_DECL_FINAL
        {
            Q_DISABLE_COPY_AND_MOVE(Parameter);

        private:
        protected:
        public:
            Parameter(
                const QString& title,
                const QString& description,
                const QString& category,
                const unsigned int nameId,
                const MapStyleValueDataType dataType,
                const QList<MapStyleConstantValue>& possibleValues);
            ~Parameter();

            QString title;
            QString description;
            QString category;
            unsigned int nameId;
            MapStyleValueDataType dataType;
            QList<MapStyleConstantValue> possibleValues;
        };

        class OSMAND_CORE_API ParameterValueDefinition : public MapStyleValueDefinition
        {
            Q_DISABLE_COPY_AND_MOVE(ParameterValueDefinition);

        private:
        protected:
        public:
            ParameterValueDefinition(
                const int id,
                const QString& name,
                const std::shared_ptr<const Parameter>& parameter);
            virtual ~ParameterValueDefinition();

            const int id;
            const std::shared_ptr<const Parameter> parameter;
        };

    private:
        PrivateImplementation<ResolvedMapStyle_P> _p;
    protected:
        ResolvedMapStyle(const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain);
    public:
        virtual ~ResolvedMapStyle();

        const QList< std::shared_ptr<const UnresolvedMapStyle> > unresolvedMapStylesChain;

        ValueDefinitionId getValueDefinitionIdByName(const QString& name) const;
        std::shared_ptr<const MapStyleValueDefinition> getValueDefinitionById(const ValueDefinitionId id) const;
        QList< std::shared_ptr<const MapStyleValueDefinition> > getValueDefinitions() const;

        bool parseValue(
            const QString& input,
            const ValueDefinitionId valueDefintionId,
            MapStyleConstantValue& outParsedValue) const;
        bool parseValue(
            const QString& input,
            const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion,
            MapStyleConstantValue& outParsedValue) const;

        const QHash<QString, QString>& constants;
        const QHash<StringId, std::shared_ptr<const Parameter> >& parameters;
        const QHash<StringId, std::shared_ptr<const Attribute> >& attributes;
        const std::array< QHash<TagValueId, std::shared_ptr<const Rule> >, MapStyleRulesetTypesCount>& rulesets;

        std::shared_ptr<const Attribute> getAttribute(const QString& name) const;
        const QHash< TagValueId, std::shared_ptr<const Rule> > getRuleset(const MapStyleRulesetType rulesetType) const;

        QString getStringById(const StringId id) const;

        QString dump(const QString& prefix = QString()) const;

        static std::shared_ptr<const ResolvedMapStyle> resolveMapStylesChain(
            const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain);
    };
}

#endif // !defined(_OSMAND_CORE_RESOLVED_MAP_STYLE_H_)
