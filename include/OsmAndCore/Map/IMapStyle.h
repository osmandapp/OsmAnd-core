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
        struct OSMAND_CORE_API Value Q_DECL_FINAL
        {
            Value();
            ~Value();

            bool isDynamic;

            MapStyleConstantValue asConstantValue;
            struct {
                std::shared_ptr<const IAttribute> attribute;
            } asDynamicValue;

            static Value fromConstantValue(const MapStyleConstantValue& input);
            static Value fromAttribute(const std::shared_ptr<const IAttribute>& attribute);
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
            virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> > getOneOfConditionalSubnodes() const = 0;
            virtual QList< std::shared_ptr<const SWIG_CLARIFY(IMapStyle, IRuleNode)> > getApplySubnodes() const = 0;
        };

        class OSMAND_CORE_API IRule
        {
            Q_DISABLE_COPY_AND_MOVE(IRule);

        private:
        protected:
            IRule();
        public:
            virtual ~IRule();

            virtual std::shared_ptr<IRuleNode> getRootNode() = 0;
            virtual std::shared_ptr<const IRuleNode> getRootNode() const = 0;
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
            virtual unsigned int getNameId() const = 0;
            virtual MapStyleValueDataType getDataType() const = 0;
            virtual QList<MapStyleConstantValue> getPossibleValues() const = 0;
        };

        class OSMAND_CORE_API IAttribute
        {
            Q_DISABLE_COPY_AND_MOVE(IAttribute);

        private:
        protected:
            IAttribute();
        public:
            virtual ~IAttribute();

            virtual std::shared_ptr<IRuleNode> getRootNode() = 0;
            virtual std::shared_ptr<const IRuleNode> getRootNode() const = 0;
            virtual StringId getNameId() const = 0;
        };

    private:
    protected:
        IMapStyle();
    public:
        virtual ~IMapStyle();

        virtual ValueDefinitionId getValueDefinitionIdByName(const QString& name) const = 0;
        virtual std::shared_ptr<const MapStyleValueDefinition> getValueDefinitionById(const ValueDefinitionId id) const = 0;
        virtual QList< std::shared_ptr<const MapStyleValueDefinition> > getValueDefinitions() const = 0;

        virtual bool parseValue(
            const QString& input,
            const ValueDefinitionId valueDefintionId,
            MapStyleConstantValue& outParsedValue) const = 0;
        virtual bool parseValue(
            const QString& input,
            const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion,
            MapStyleConstantValue& outParsedValue) const = 0;

        virtual std::shared_ptr<const IParameter> getParameter(const QString& name) const = 0;
        virtual QList< std::shared_ptr<const IParameter> > getParameters() const = 0;
        virtual std::shared_ptr<const IAttribute> getAttribute(const QString& name) const = 0;
        virtual QList< std::shared_ptr<const IAttribute> > getAttributes() const = 0;
        virtual QHash< TagValueId, std::shared_ptr<const IRule> > getRuleset(const MapStyleRulesetType rulesetType) const = 0;

        virtual QString getStringById(const StringId id) const = 0;
    };
}

#endif // !defined(_OSMAND_CORE_I_MAP_STYLE_H_)
