#ifndef _OSMAND_CORE_I_MAP_STYLE_H_
#define _OSMAND_CORE_I_MAP_STYLE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonSWIG.h>
#include <OsmAndCore/Map/MapCommonTypes.h>
#include <OsmAndCore/Map/MapStyleConstantValue.h>

namespace OsmAnd
{
    class MapStyleValueDefinition;

    class OSMAND_CORE_API IMapStyle
    {
        Q_DISABLE_COPY_AND_MOVE(IMapStyle);
    public:
        typedef unsigned int StringId;
        enum : unsigned int {
            EmptyStringId = 0u
        };

        typedef int ValueDefinitionId;

        class IAttribute;
        class ISymbolClass;
        struct OSMAND_CORE_API Value Q_DECL_FINAL
        {
            Value();
            ~Value();

            bool isDynamic;

            MapStyleConstantValue asConstantValue;
            struct {
                std::shared_ptr<const IAttribute> attribute;
                std::shared_ptr<QList<std::shared_ptr<const ISymbolClass>>> symbolClasses;
                std::shared_ptr<QList<StringId>> symbolClassTemplates;
            } asDynamicValue;

            static Value fromConstantValue(const MapStyleConstantValue& input);
            static Value fromAttribute(const std::shared_ptr<const IAttribute>& attribute);
            static Value fromSymbolClass(const std::shared_ptr<const ISymbolClass>& symbolClass);
        };

        class OSMAND_CORE_API IRuleNode
        {
            Q_DISABLE_COPY_AND_MOVE(IRuleNode);

        private:
        protected:
            IRuleNode();
        public:
            virtual ~IRuleNode();

            virtual bool getIsSwitch() const = 0;

            virtual QHash<SWIG_CLARIFY(IMapStyle, ValueDefinitionId), SWIG_CLARIFY(IMapStyle, Value)> getValues() const = 0;
            virtual const QHash<SWIG_CLARIFY(IMapStyle, ValueDefinitionId), SWIG_CLARIFY(IMapStyle, Value)>&
                getValuesRef() const = 0;
            virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> > getOneOfConditionalSubnodes() const = 0;
            virtual const QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> >&
                getOneOfConditionalSubnodesRef() const = 0;
            virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> > getApplySubnodes() const = 0;
            virtual const QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> >& getApplySubnodesRef() const = 0;
        };

        class OSMAND_CORE_API IRule
        {
            Q_DISABLE_COPY_AND_MOVE(IRule);

        private:
        protected:
            IRule();
        public:
            virtual ~IRule();

            virtual std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() = 0;
            virtual const std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() = 0;
            virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() const = 0;
            virtual const std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() const = 0;
            virtual MapStyleRulesetType getRulesetType() const = 0;
        };

        class OSMAND_CORE_API IParameter
        {
            Q_DISABLE_COPY_AND_MOVE(IParameter);

        private:
        protected:
            IParameter();
        public:
            virtual ~IParameter();

            virtual QString getTitle() const = 0;
            virtual QString getDescription() const = 0;
            virtual QString getCategory() const = 0;
            virtual SWIG_CLARIFY(IMapStyle, StringId) getNameId() const = 0;
            virtual MapStyleValueDataType getDataType() const = 0;
            virtual QList<MapStyleConstantValue> getPossibleValues() const = 0;
            virtual QString getDefaultValueDescription() const = 0;
        };

        class OSMAND_CORE_API IAttribute
        {
            Q_DISABLE_COPY_AND_MOVE(IAttribute);

        private:
        protected:
            IAttribute();
        public:
            virtual ~IAttribute();

            virtual std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() = 0;
            virtual const std::shared_ptr<SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() = 0;
            virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> getRootNode() const = 0;
            virtual const std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)>& getRootNodeRef() const = 0;
            virtual SWIG_CLARIFY(IMapStyle, StringId) getNameId() const = 0;
        };

        class OSMAND_CORE_API ISymbolClass
        {
            Q_DISABLE_COPY_AND_MOVE(ISymbolClass);

        private:
        protected:
            ISymbolClass();
        public:
            virtual ~ISymbolClass();

            virtual QString getTitle() const = 0;
            virtual QString getDescription() const = 0;
            virtual QString getCategory() const = 0;
            virtual QString getLegendObject() const = 0;
            virtual QString getInnerLegendObject() const = 0;
            virtual QString getInnerTitle() const = 0;
            virtual QString getInnerDescription() const = 0;
            virtual QString getInnerCategory() const = 0;
            virtual QString getInnerNames() const = 0;
            virtual bool getDefaultSetting() const = 0;
            virtual SWIG_CLARIFY(IMapStyle, StringId) getNameId() const = 0;
        };

    private:
    protected:
        IMapStyle();
    public:
        virtual ~IMapStyle();

        virtual SWIG_CLARIFY(IMapStyle, ValueDefinitionId) getValueDefinitionIdByNameId(
            const StringId& nameId) const = 0;
        virtual SWIG_CLARIFY(IMapStyle, ValueDefinitionId) getValueDefinitionIdByName(const QString& name) const = 0;
        virtual std::shared_ptr<const MapStyleValueDefinition> getValueDefinitionById(
            const SWIG_CLARIFY(IMapStyle, ValueDefinitionId) id) const = 0;
        virtual const std::shared_ptr<const MapStyleValueDefinition>& getValueDefinitionRefById(
            const SWIG_CLARIFY(IMapStyle, ValueDefinitionId) id) const = 0;
        virtual QList< std::shared_ptr<const MapStyleValueDefinition> > getValueDefinitions() const = 0;
        virtual int getValueDefinitionsCount() const = 0;

        virtual bool parseValue(
            const QString& input,
            const SWIG_CLARIFY(IMapStyle, ValueDefinitionId) valueDefintionId,
            MapStyleConstantValue& outParsedValue) const = 0;
        virtual bool parseValue(
            const QString& input,
            const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion,
            MapStyleConstantValue& outParsedValue) const = 0;

        virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IParameter)> getParameter(const QString& name) const = 0;
        virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IParameter)> > getParameters() const = 0;
        virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IAttribute)> getAttribute(const QString& name) const = 0;
        virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IAttribute)> > getAttributes() const = 0;
        virtual std::shared_ptr<const SWIG_CLARIFY(IMapStyle, ISymbolClass)> getSymbolClass(const QString& name) const = 0;
        virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, ISymbolClass)> > getSymbolClasses() const = 0;
        virtual QHash< TagValueId, std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRule)> > getRuleset(
            const MapStyleRulesetType rulesetType) const = 0;

        virtual QString getStringById(const SWIG_CLARIFY(IMapStyle, StringId) id) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_STYLE_H_)
