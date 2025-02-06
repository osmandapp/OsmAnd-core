#include "ResolvedMapStyle.h"
#include "ResolvedMapStyle_P.h"

#include "QtExtensions.h"
#include "QtCommon.h"

OsmAnd::ResolvedMapStyle::ResolvedMapStyle(const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain_)
    : _p(new ResolvedMapStyle_P(this))
    , unresolvedMapStylesChain(unresolvedMapStylesChain_)
{
}

OsmAnd::ResolvedMapStyle::~ResolvedMapStyle()
{
}

OsmAnd::IMapStyle::ValueDefinitionId OsmAnd::ResolvedMapStyle::getValueDefinitionIdByNameId(
    const StringId& nameId) const
{
    return _p->getValueDefinitionIdByNameId(nameId);
}

OsmAnd::IMapStyle::ValueDefinitionId OsmAnd::ResolvedMapStyle::getValueDefinitionIdByName(
    const QString& name) const
{
    return _p->getValueDefinitionIdByName(name);
}

std::shared_ptr<const OsmAnd::MapStyleValueDefinition> OsmAnd::ResolvedMapStyle::getValueDefinitionById(
    const ValueDefinitionId id) const
{
    return _p->getValueDefinitionById(id);
}

const std::shared_ptr<const OsmAnd::MapStyleValueDefinition>& OsmAnd::ResolvedMapStyle::getValueDefinitionRefById(
    const ValueDefinitionId id) const
{
    return _p->getValueDefinitionRefById(id);
}

QList< std::shared_ptr<const OsmAnd::MapStyleValueDefinition> > OsmAnd::ResolvedMapStyle::getValueDefinitions() const
{
    return _p->getValueDefinitions();
}

int OsmAnd::ResolvedMapStyle::getValueDefinitionsCount() const
{
    return _p->getValueDefinitionsCount();
}

bool OsmAnd::ResolvedMapStyle::parseValue(
    const QString& input,
    const ValueDefinitionId valueDefintionId,
    MapStyleConstantValue& outParsedValue) const
{
    return _p->parseConstantValue(input, valueDefintionId, outParsedValue);
}

bool OsmAnd::ResolvedMapStyle::parseValue(
    const QString& input,
    const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion,
    MapStyleConstantValue& outParsedValue) const
{
    return _p->parseConstantValue(input, valueDefintion, outParsedValue);
}

std::shared_ptr<const OsmAnd::IMapStyle::IParameter> OsmAnd::ResolvedMapStyle::getParameter(
    const QString& name) const
{
    return _p->getParameter(name);
}

QList< std::shared_ptr<const OsmAnd::IMapStyle::IParameter> > OsmAnd::ResolvedMapStyle::getParameters() const
{
    return _p->getParameters();
}

std::shared_ptr<const OsmAnd::IMapStyle::IAttribute> OsmAnd::ResolvedMapStyle::getAttribute(
    const QString& name) const
{
    return _p->getAttribute(name);
}

QList< std::shared_ptr<const OsmAnd::IMapStyle::IAttribute> > OsmAnd::ResolvedMapStyle::getAttributes() const
{
    return _p->getAttributes();
}

std::shared_ptr<const OsmAnd::IMapStyle::ISymbolClass> OsmAnd::ResolvedMapStyle::getSymbolClass(
    const QString& name) const
{
    return _p->getSymbolClass(name);
}

QList< std::shared_ptr<const OsmAnd::IMapStyle::ISymbolClass> > OsmAnd::ResolvedMapStyle::getSymbolClasses() const
{
    return _p->getSymbolClasses();
}

QHash< OsmAnd::TagValueId, std::shared_ptr<const OsmAnd::IMapStyle::IRule> > OsmAnd::ResolvedMapStyle::getRuleset(
    const MapStyleRulesetType rulesetType) const
{
    return _p->getRuleset(rulesetType);
}

QString OsmAnd::ResolvedMapStyle::getStringById(const StringId id) const
{
    return _p->getStringById(id);
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle> OsmAnd::ResolvedMapStyle::resolveMapStylesChain(
    const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain)
{
    const std::shared_ptr<ResolvedMapStyle> resolvedStyle(new ResolvedMapStyle(unresolvedMapStylesChain));

    if (!resolvedStyle->_p->resolve())
        return nullptr;

    return resolvedStyle;
}

OsmAnd::ResolvedMapStyle::RuleNode::RuleNode(const bool isSwitch_)
    : isSwitch(isSwitch_)
{
}

OsmAnd::ResolvedMapStyle::RuleNode::~RuleNode()
{
}

bool OsmAnd::ResolvedMapStyle::RuleNode::getIsSwitch() const
{
    return isSwitch;
}

QHash<OsmAnd::IMapStyle::ValueDefinitionId, OsmAnd::IMapStyle::Value> OsmAnd::ResolvedMapStyle::RuleNode::getValues() const
{
    return values;
}

const QHash<OsmAnd::IMapStyle::ValueDefinitionId, OsmAnd::IMapStyle::Value>&
OsmAnd::ResolvedMapStyle::RuleNode::getValuesRef() const
{
    return values;
}

QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> >
OsmAnd::ResolvedMapStyle::RuleNode::getOneOfConditionalSubnodes() const
{
    return oneOfConditionalSubnodes;
}

const QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> >&
OsmAnd::ResolvedMapStyle::RuleNode::getOneOfConditionalSubnodesRef() const
{
    return oneOfConditionalSubnodes;
}

QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> >
OsmAnd::ResolvedMapStyle::RuleNode::getApplySubnodes() const
{
    return applySubnodes;
}

const QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> >&
OsmAnd::ResolvedMapStyle::RuleNode::getApplySubnodesRef() const
{
    return applySubnodes;
}

OsmAnd::ResolvedMapStyle::BaseRule::BaseRule(RuleNode* const ruleNode_)
    : rootNode(ruleNode_)
    , rootNodeAsInterface(rootNode)
    , rootNodeAsConstInterface(rootNode)
{
}

OsmAnd::ResolvedMapStyle::BaseRule::~BaseRule()
{
}

OsmAnd::ResolvedMapStyle::Rule::Rule(const MapStyleRulesetType rulesetType_)
    : BaseRule(new RuleNode(true))
    , rulesetType(rulesetType_)
{
}

OsmAnd::ResolvedMapStyle::Rule::~Rule()
{
}

std::shared_ptr<OsmAnd::IMapStyle::IRuleNode> OsmAnd::ResolvedMapStyle::Rule::getRootNode()
{
    return rootNodeAsInterface;
}

const std::shared_ptr<OsmAnd::IMapStyle::IRuleNode>& OsmAnd::ResolvedMapStyle::Rule::getRootNodeRef()
{
    return rootNodeAsInterface;
}

std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> OsmAnd::ResolvedMapStyle::Rule::getRootNode() const
{
    return rootNodeAsConstInterface;
}

const std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode>& OsmAnd::ResolvedMapStyle::Rule::getRootNodeRef() const
{
    return rootNodeAsConstInterface;
}

OsmAnd::MapStyleRulesetType OsmAnd::ResolvedMapStyle::Rule::getRulesetType() const
{
    return rulesetType;
}

OsmAnd::ResolvedMapStyle::Attribute::Attribute(const StringId nameId_)
    : BaseRule(new RuleNode(false))
    , nameId(nameId_)
{
}

OsmAnd::ResolvedMapStyle::Attribute::~Attribute()
{
}

std::shared_ptr<OsmAnd::IMapStyle::IRuleNode> OsmAnd::ResolvedMapStyle::Attribute::getRootNode()
{
    return rootNodeAsInterface;
}

const std::shared_ptr<OsmAnd::IMapStyle::IRuleNode>& OsmAnd::ResolvedMapStyle::Attribute::getRootNodeRef()
{
    return rootNodeAsInterface;
}

std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> OsmAnd::ResolvedMapStyle::Attribute::getRootNode() const
{
    return rootNodeAsConstInterface;
}

const std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode>& OsmAnd::ResolvedMapStyle::Attribute::getRootNodeRef() const
{
    return rootNodeAsConstInterface;
}

OsmAnd::IMapStyle::StringId OsmAnd::ResolvedMapStyle::Attribute::getNameId() const
{
    return nameId;
}

OsmAnd::ResolvedMapStyle::Parameter::Parameter(
    const QString& title_,
    const QString& description_,
    const QString& category_,
    const StringId nameId_,
    const MapStyleValueDataType dataType_,
    const QList<MapStyleConstantValue>& possibleValues_,
    const QString& defaultValueDescription_)
    : title(title_)
    , description(description_)
    , category(category_)
    , nameId(nameId_)
    , dataType(dataType_)
    , possibleValues(possibleValues_)
    , defaultValueDescription(defaultValueDescription_)
{
}

OsmAnd::ResolvedMapStyle::Parameter::~Parameter()
{
}

QString OsmAnd::ResolvedMapStyle::Parameter::getTitle() const
{
    return title;
}

QString OsmAnd::ResolvedMapStyle::Parameter::getDescription() const
{
    return description;
}

QString OsmAnd::ResolvedMapStyle::Parameter::getCategory() const
{
    return category;
}

OsmAnd::IMapStyle::StringId OsmAnd::ResolvedMapStyle::Parameter::getNameId() const
{
    return nameId;
}

OsmAnd::MapStyleValueDataType OsmAnd::ResolvedMapStyle::Parameter::getDataType() const
{
    return dataType;
}

QList<OsmAnd::MapStyleConstantValue> OsmAnd::ResolvedMapStyle::Parameter::getPossibleValues() const
{
    return possibleValues;
}

QString OsmAnd::ResolvedMapStyle::Parameter::getDefaultValueDescription() const
{
    return defaultValueDescription;
}

OsmAnd::ResolvedMapStyle::SymbolClass::SymbolClass(
    const QString& title_,
    const QString& description_,
    const QString& category_,
    const QString& legendObject_,
    const QString& innerLegendObject_,
    const QString& innerTitle_,
    const QString& innerDescription_,
    const QString& innerCategory_,
    const QString& innerNames_,
    const bool isSetByDefault_,
    const StringId nameId_)
    : title(title_)
    , description(description_)
    , category(category_)
    , legendObject(legendObject_)
    , innerLegendObject(innerLegendObject_)
    , innerTitle(innerTitle_)
    , innerDescription(innerDescription_)
    , innerCategory(innerCategory_)
    , innerNames(innerNames_)
    , isSetByDefault(isSetByDefault_)
    , nameId(nameId_)
{
}

OsmAnd::ResolvedMapStyle::SymbolClass::~SymbolClass()
{
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getTitle() const
{
    return title;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getDescription() const
{
    return description;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getCategory() const
{
    return category;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getLegendObject() const
{
    return legendObject;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getInnerLegendObject() const
{
    return innerLegendObject;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getInnerTitle() const
{
    return innerTitle;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getInnerDescription() const
{
    return innerDescription;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getInnerCategory() const
{
    return innerCategory;
}

QString OsmAnd::ResolvedMapStyle::SymbolClass::getInnerNames() const
{
    return innerNames;
}

bool OsmAnd::ResolvedMapStyle::SymbolClass::getDefaultSetting() const
{
    return isSetByDefault;
}

OsmAnd::IMapStyle::StringId OsmAnd::ResolvedMapStyle::SymbolClass::getNameId() const
{
    return nameId;
}

OsmAnd::ResolvedMapStyle::ParameterValueDefinition::ParameterValueDefinition(
    const int id_,
    const QString& name_,
    const std::shared_ptr<const Parameter>& parameter_)
    : MapStyleValueDefinition(MapStyleValueDefinition::Class::Input, parameter_->dataType, name_, false)
    , id(id_)
    , parameter(parameter_)
{
}

OsmAnd::ResolvedMapStyle::ParameterValueDefinition::~ParameterValueDefinition()
{
}

OsmAnd::ResolvedMapStyle::SymbolClassValueDefinition::SymbolClassValueDefinition(
    const int id_,
    const QString& name_,
    const std::shared_ptr<const SymbolClass>& symbolClass_)
    : MapStyleValueDefinition(MapStyleValueDefinition::Class::Input, MapStyleValueDataType::Boolean, name_, false)
    , id(id_)
    , symbolClass(symbolClass_)
{
}

OsmAnd::ResolvedMapStyle::SymbolClassValueDefinition::~SymbolClassValueDefinition()
{
}
