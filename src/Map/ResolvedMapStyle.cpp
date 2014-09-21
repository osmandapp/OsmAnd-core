#include "ResolvedMapStyle.h"
#include "ResolvedMapStyle_P.h"

OsmAnd::ResolvedMapStyle::ResolvedMapStyle(const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain_)
    : _p(new ResolvedMapStyle_P(this))
    , unresolvedMapStylesChain(unresolvedMapStylesChain_)
{
}

OsmAnd::ResolvedMapStyle::~ResolvedMapStyle()
{
}

OsmAnd::ResolvedMapStyle::ValueDefinitionId OsmAnd::ResolvedMapStyle::getValueDefinitionIdByName(const QString& name) const
{
    return _p->getValueDefinitionIdByName(name);
}

std::shared_ptr<const OsmAnd::MapStyleValueDefinition> OsmAnd::ResolvedMapStyle::getValueDefinitionById(const ValueDefinitionId id) const
{
    return _p->getValueDefinitionById(id);
}

bool OsmAnd::ResolvedMapStyle::parseValue(const QString& input, const ValueDefinitionId valueDefintionId, MapStyleConstantValue& outParsedValue) const
{
    return _p->parseConstantValue(input, valueDefintionId, outParsedValue);
}

bool OsmAnd::ResolvedMapStyle::parseValue(const QString& input, const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion, MapStyleConstantValue& outParsedValue) const
{
    return _p->parseConstantValue(input, valueDefintion, outParsedValue);
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle::Attribute> OsmAnd::ResolvedMapStyle::getAttribute(const QString& name) const
{
    return _p->getAttribute(name);
}

const QHash< OsmAnd::TagValueId, std::shared_ptr<const OsmAnd::ResolvedMapStyle::Rule> >
OsmAnd::ResolvedMapStyle::getRuleset(
    const MapStyleRulesetType rulesetType) const
{
    return _p->getRuleset(rulesetType);
}

QString OsmAnd::ResolvedMapStyle::getStringById(const StringId id) const
{
    return _p->getStringById(id);
}

QString OsmAnd::ResolvedMapStyle::dump(const QString& prefix /*= QString()*/) const
{
    return _p->dump(prefix);
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle> OsmAnd::ResolvedMapStyle::resolveMapStylesChain(const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain)
{
    const std::shared_ptr<ResolvedMapStyle> resolvedStyle(new ResolvedMapStyle(unresolvedMapStylesChain));

    if (!resolvedStyle->_p->resolve())
        return nullptr;

    return resolvedStyle;
}

OsmAnd::ResolvedMapStyle::ResolvedValue::ResolvedValue()
    : isDynamic(false)
{
}

OsmAnd::ResolvedMapStyle::ResolvedValue::~ResolvedValue()
{
}

OsmAnd::ResolvedMapStyle::ResolvedValue OsmAnd::ResolvedMapStyle::ResolvedValue::fromConstantValue(const MapStyleConstantValue& input)
{
    ResolvedValue value;
    value.isDynamic = false;
    value.asConstantValue = input;
    return value;
}

OsmAnd::ResolvedMapStyle::ResolvedValue OsmAnd::ResolvedMapStyle::ResolvedValue::fromAttribute(const std::shared_ptr<const Attribute>& attribute)
{
    ResolvedValue value;
    value.isDynamic = true;
    value.asDynamicValue.attribute = attribute;
    return value;
}

OsmAnd::ResolvedMapStyle::RuleNode::RuleNode(const bool applyOnlyIfOneOfConditionalsAccepted_)
    : applyOnlyIfOneOfConditionalsAccepted(applyOnlyIfOneOfConditionalsAccepted_)
{
}

OsmAnd::ResolvedMapStyle::RuleNode::~RuleNode()
{
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

OsmAnd::ResolvedMapStyle::Attribute::Attribute(const StringId nameId_)
    : BaseRule(new RuleNode(false))
    , nameId(nameId_)
{
}

OsmAnd::ResolvedMapStyle::Attribute::~Attribute()
{
}

OsmAnd::ResolvedMapStyle::Parameter::Parameter(
    const QString& title_,
    const QString& description_,
    const unsigned int nameId_,
    const MapStyleValueDataType dataType_,
    const QList<MapStyleConstantValue>& possibleValues_)
    : title(title_)
    , description(description_)
    , nameId(nameId_)
    , dataType(dataType_)
    , possibleValues(possibleValues_)
{
}

OsmAnd::ResolvedMapStyle::Parameter::~Parameter()
{
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
