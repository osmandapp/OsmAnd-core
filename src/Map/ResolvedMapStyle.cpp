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

QList< std::shared_ptr<const OsmAnd::MapStyleValueDefinition> > OsmAnd::ResolvedMapStyle::getValueDefinitions() const
{
    return _p->getValueDefinitions();
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

std::shared_ptr<const OsmAnd::IMapStyle::IAttribute> OsmAnd::ResolvedMapStyle::getAttribute(
    const QString& name) const
{
    return _p->getAttribute(name);
}

QHash< OsmAnd::TagValueId, std::shared_ptr<const OsmAnd::IMapStyle::IRule> > OsmAnd::ResolvedMapStyle::getRuleset(
    const MapStyleRulesetType rulesetType) const
{
    return copyAs< TagValueId, std::shared_ptr<const IRule> >(_p->getRuleset(rulesetType));
}

QString OsmAnd::ResolvedMapStyle::getStringById(const StringId id) const
{
    return _p->getStringById(id);
}

QString OsmAnd::ResolvedMapStyle::dump(const QString& prefix /*= QString()*/) const
{
    return _p->dump(prefix);
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

QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> >
OsmAnd::ResolvedMapStyle::RuleNode::getOneOfConditionalSubnodes() const
{
    return copyAs< QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> > >(oneOfConditionalSubnodes);
}

QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> >
OsmAnd::ResolvedMapStyle::RuleNode::getApplySubnodes() const
{
    return copyAs< QList< std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> > >(applySubnodes);
}

OsmAnd::ResolvedMapStyle::BaseRule::BaseRule(RuleNode* const ruleNode_)
    : rootNode(ruleNode_)
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
    return rootNode;
}

std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> OsmAnd::ResolvedMapStyle::Rule::getRootNode() const
{
    return rootNode;
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
    return rootNode;
}

std::shared_ptr<const OsmAnd::IMapStyle::IRuleNode> OsmAnd::ResolvedMapStyle::Attribute::getRootNode() const
{
    return rootNode;
}

OsmAnd::IMapStyle::StringId OsmAnd::ResolvedMapStyle::Attribute::getNameId() const
{
    return nameId;
}

OsmAnd::ResolvedMapStyle::Parameter::Parameter(
    const QString& title_,
    const QString& description_,
    const QString& category_,
    const unsigned int nameId_,
    const MapStyleValueDataType dataType_,
    const QList<MapStyleConstantValue>& possibleValues_)
    : title(title_)
    , description(description_)
    , category(category_)
    , nameId(nameId_)
    , dataType(dataType_)
    , possibleValues(possibleValues_)
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

unsigned int OsmAnd::ResolvedMapStyle::Parameter::getNameId() const
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
