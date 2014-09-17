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

bool OsmAnd::ResolvedMapStyle::parseValue(const QString& input, const ValueDefinitionId valueDefintionId, MapStyleValue& outParsedValue) const
{
    return _p->parseValue(input, valueDefintionId, outParsedValue);
}

bool OsmAnd::ResolvedMapStyle::parseValue(const QString& input, const std::shared_ptr<const MapStyleValueDefinition>& valueDefintion, MapStyleValue& outParsedValue) const
{
    return _p->parseValue(input, valueDefintion, outParsedValue);
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle::Attribute> OsmAnd::ResolvedMapStyle::getAttribute(const QString& name) const
{
    return _p->getAttribute(name);
}

const QHash< OsmAnd::ResolvedMapStyle::TagValueId, std::shared_ptr<const OsmAnd::ResolvedMapStyle::Rule> >
OsmAnd::ResolvedMapStyle::getRuleset(
    const MapStyleRulesetType rulesetType) const
{
    return _p->getRuleset(rulesetType);
}

QString OsmAnd::ResolvedMapStyle::getStringById(const StringId id) const
{
    return _p->getStringById(id);
}

std::shared_ptr<const OsmAnd::ResolvedMapStyle> OsmAnd::ResolvedMapStyle::resolveMapStylesChain(const QList< std::shared_ptr<const UnresolvedMapStyle> >& unresolvedMapStylesChain)
{
    const std::shared_ptr<ResolvedMapStyle> resolvedStyle(new ResolvedMapStyle(unresolvedMapStylesChain));

    if (!resolvedStyle->_p->resolve())
        return nullptr;

    return resolvedStyle;
}

OsmAnd::ResolvedMapStyle::RuleNode::RuleNode()
{
}

OsmAnd::ResolvedMapStyle::RuleNode::~RuleNode()
{
}

OsmAnd::ResolvedMapStyle::BaseRule::BaseRule()
    : rootNode(new RuleNode())
{
}

OsmAnd::ResolvedMapStyle::BaseRule::~BaseRule()
{
}

OsmAnd::ResolvedMapStyle::Rule::Rule(const MapStyleRulesetType rulesetType_)
    : rulesetType(rulesetType_)
{
}

OsmAnd::ResolvedMapStyle::Rule::~Rule()
{
}

OsmAnd::ResolvedMapStyle::Attribute::Attribute(const StringId nameId_)
    : nameId(nameId_)
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
    const QList<MapStyleValue>& possibleValues_)
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
