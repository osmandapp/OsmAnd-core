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
        class Attribute;

        class OSMAND_CORE_API RuleNode Q_DECL_FINAL : public IMapStyle::IRuleNode
        {
            Q_DISABLE_COPY_AND_MOVE(RuleNode);

        private:
        protected:
        public:
            RuleNode(const bool isSwitch);
            ~RuleNode();

#if !defined(SWIG)
            const bool isSwitch;
#endif // !defined(SWIG)
            virtual bool getIsSwitch() const Q_DECL_OVERRIDE;

#if !defined(SWIG)
            QHash<ValueDefinitionId, Value> values;
            QList< std::shared_ptr<const IMapStyle::IRuleNode> > oneOfConditionalSubnodes;
            QList< std::shared_ptr<const IMapStyle::IRuleNode> > applySubnodes;
#endif // !defined(SWIG)
            virtual QHash<SWIG_CLARIFY(IMapStyle, ValueDefinitionId), SWIG_CLARIFY(IMapStyle, Value)>
                getValues() const Q_DECL_OVERRIDE;
            virtual const QHash<SWIG_CLARIFY(IMapStyle, ValueDefinitionId), SWIG_CLARIFY(IMapStyle, Value)>&
                getValuesRef() const Q_DECL_OVERRIDE;
            virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> >
                getOneOfConditionalSubnodes() const Q_DECL_OVERRIDE;
            virtual const QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> >&
                getOneOfConditionalSubnodesRef() const Q_DECL_OVERRIDE;
            virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> > getApplySubnodes() const Q_DECL_OVERRIDE;
            virtual const QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> >&
                getApplySubnodesRef() const Q_DECL_OVERRIDE;
        };

        class OSMAND_CORE_API BaseRule
        {
            Q_DISABLE_COPY_AND_MOVE(BaseRule);

        private:
        protected:
            BaseRule(RuleNode* const ruleNode);
        public:
            virtual ~BaseRule();

#if !defined(SWIG)
            const std::shared_ptr<RuleNode> rootNode;
#endif // !defined(SWIG)
            const std::shared_ptr<IMapStyle::IRuleNode> rootNodeAsInterface;
            const std::shared_ptr<const IMapStyle::IRuleNode> rootNodeAsConstInterface;
        };

        class OSMAND_CORE_API Rule Q_DECL_FINAL
            : public BaseRule
            , public IMapStyle::IRule
        {
            Q_DISABLE_COPY_AND_MOVE(Rule);

        private:
        protected:
        public:
            Rule(const MapStyleRulesetType rulesetType);
            virtual ~Rule();

            virtual std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() Q_DECL_OVERRIDE;
            virtual const std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() Q_DECL_OVERRIDE;
            virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() const Q_DECL_OVERRIDE;
            virtual const std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() const Q_DECL_OVERRIDE;

#if !defined(SWIG)
            const MapStyleRulesetType rulesetType;
#endif // !defined(SWIG)
            virtual MapStyleRulesetType getRulesetType() const Q_DECL_OVERRIDE;
        };

        class OSMAND_CORE_API Attribute Q_DECL_FINAL
            : public BaseRule
            , public IMapStyle::IAttribute
        {
            Q_DISABLE_COPY_AND_MOVE(Attribute);

        private:
        protected:
        public:
            Attribute(const SWIG_CLARIFY(IMapStyle, StringId) nameId);
            virtual ~Attribute();

            virtual std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() Q_DECL_OVERRIDE;
            virtual const std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() Q_DECL_OVERRIDE;
            virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() const Q_DECL_OVERRIDE;
            virtual const std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() const Q_DECL_OVERRIDE;

#if !defined(SWIG)
            const StringId nameId;
#endif // !defined(SWIG)
            virtual SWIG_CLARIFY(IMapStyle, StringId) getNameId() const Q_DECL_OVERRIDE;
        };
        
        class OSMAND_CORE_API Parameter Q_DECL_FINAL
            : public IMapStyle::IParameter
        {
            Q_DISABLE_COPY_AND_MOVE(Parameter);

        private:
        protected:
        public:
            Parameter(
                const QString& title,
                const QString& description,
                const QString& category,
                const SWIG_CLARIFY(IMapStyle, StringId) nameId,
                const MapStyleValueDataType dataType,
                const QList<MapStyleConstantValue>& possibleValues,
                const QString& defaultValueDescription);
            virtual ~Parameter();

#if !defined(SWIG)
            QString title;
            QString description;
            QString category;
            StringId nameId;
            MapStyleValueDataType dataType;
            QList<MapStyleConstantValue> possibleValues;
            QString defaultValueDescription;
#endif // !defined(SWIG)
            virtual QString getTitle() const Q_DECL_OVERRIDE;
            virtual QString getDescription() const Q_DECL_OVERRIDE;
            virtual QString getCategory() const Q_DECL_OVERRIDE;
            virtual SWIG_CLARIFY(IMapStyle, StringId) getNameId() const Q_DECL_OVERRIDE;
            virtual MapStyleValueDataType getDataType() const Q_DECL_OVERRIDE;
            virtual QList<MapStyleConstantValue> getPossibleValues() const Q_DECL_OVERRIDE;
            virtual QString getDefaultValueDescription() const Q_DECL_OVERRIDE;
        };

        class OSMAND_CORE_API SymbolClass Q_DECL_FINAL
            : public IMapStyle::ISymbolClass
        {
            Q_DISABLE_COPY_AND_MOVE(SymbolClass);

        private:
        protected:
        public:
            SymbolClass(
                const QString& title,
                const QString& description,
                const QString& category,
                const bool isSetByDefault,
                const SWIG_CLARIFY(IMapStyle, StringId) nameId);
            virtual ~SymbolClass();

#if !defined(SWIG)
            QString title;
            QString description;
            QString category;
            StringId nameId;
            bool isSetByDefault;
#endif // !defined(SWIG)
            virtual QString getTitle() const Q_DECL_OVERRIDE;
            virtual QString getDescription() const Q_DECL_OVERRIDE;
            virtual QString getCategory() const Q_DECL_OVERRIDE;
            virtual bool getDefaultSetting() const Q_DECL_OVERRIDE;
            virtual SWIG_CLARIFY(IMapStyle, StringId) getNameId() const Q_DECL_OVERRIDE;
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

        class OSMAND_CORE_API SymbolClassValueDefinition : public MapStyleValueDefinition
        {
            Q_DISABLE_COPY_AND_MOVE(SymbolClassValueDefinition);

        private:
        protected:
        public:
            SymbolClassValueDefinition(
                const int id,
                const QString& name,
                const std::shared_ptr<const SymbolClass>& symbolClass);
            virtual ~SymbolClassValueDefinition();

            const int id;
            const std::shared_ptr<const SymbolClass> symbolClass;
        };

    private:
        PrivateImplementation<ResolvedMapStyle_P> _p;
    protected:
        ResolvedMapStyle(const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain);
    public:
        virtual ~ResolvedMapStyle();

        const QList< std::shared_ptr<const UnresolvedMapStyle> > unresolvedMapStylesChain;

        virtual SWIG_CLARIFY(IMapStyle, ValueDefinitionId) getValueDefinitionIdByNameId(
            const StringId& nameId) const Q_DECL_OVERRIDE;
        virtual SWIG_CLARIFY(IMapStyle, ValueDefinitionId) getValueDefinitionIdByName(
            const QString& name) const Q_DECL_OVERRIDE;
        virtual std::shared_ptr<const MapStyleValueDefinition> getValueDefinitionById(
            const SWIG_CLARIFY(IMapStyle, ValueDefinitionId) id) const Q_DECL_OVERRIDE;
        virtual const std::shared_ptr<const MapStyleValueDefinition>& getValueDefinitionRefById(
            const SWIG_CLARIFY(IMapStyle, ValueDefinitionId) id) const Q_DECL_OVERRIDE;
        virtual QList< std::shared_ptr<const MapStyleValueDefinition> > getValueDefinitions() const Q_DECL_OVERRIDE;
        virtual int getValueDefinitionsCount() const Q_DECL_OVERRIDE;

        virtual bool parseValue(
            const QString& input,
            const SWIG_CLARIFY(IMapStyle, ValueDefinitionId) valueDefintionId,
            MapStyleConstantValue& outParsedValue) const Q_DECL_OVERRIDE;
        virtual bool parseValue(
            const QString& input,
            const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion,
            MapStyleConstantValue& outParsedValue) const Q_DECL_OVERRIDE;

        virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IParameter)> getParameter(
            const QString& name) const Q_DECL_OVERRIDE;
        virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IParameter)> > getParameters() const Q_DECL_OVERRIDE;
        virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IAttribute)> getAttribute(
            const QString& name) const Q_DECL_OVERRIDE;
        virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IAttribute)> > getAttributes() const Q_DECL_OVERRIDE;
        virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, ISymbolClass)> getSymbolClass(
            const QString& name) const Q_DECL_OVERRIDE;
        virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, ISymbolClass)> > getSymbolClasses() const Q_DECL_OVERRIDE;
        virtual QHash< TagValueId, std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRule)> > getRuleset(
            const MapStyleRulesetType rulesetType) const Q_DECL_OVERRIDE;

        virtual QString getStringById(const SWIG_CLARIFY(IMapStyle, StringId) id) const Q_DECL_OVERRIDE;

        static std::shared_ptr<const ResolvedMapStyle> resolveMapStylesChain(
            const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain);
    };
}

#endif // !defined(_OSMAND_CORE_RESOLVED_MAP_STYLE_H_)
