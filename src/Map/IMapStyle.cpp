#include "IMapStyle.h"

OsmAnd::IMapStyle::IMapStyle()
{
}

OsmAnd::IMapStyle::~IMapStyle()
{
}

OsmAnd::IMapStyle::Value::Value()
    : isDynamic(false)
{
}

OsmAnd::IMapStyle::Value::~Value()
{
}

OsmAnd::IMapStyle::Value OsmAnd::IMapStyle::Value::fromConstantValue(
    const MapStyleConstantValue& input)
{
    Value value;
    value.isDynamic = false;
    value.asConstantValue = input;
    return value;
}

OsmAnd::IMapStyle::Value OsmAnd::IMapStyle::Value::fromAttribute(
    const std::shared_ptr<const IAttribute>& attribute)
{
    Value value;
    value.isDynamic = true;
    value.asDynamicValue.attribute = attribute;
    return value;
}

OsmAnd::IMapStyle::Value OsmAnd::IMapStyle::Value::fromSymbolClass(
    const std::shared_ptr<const ISymbolClass>& symbolClass)
{
    const auto symbolClasses = std::make_shared<QList<std::shared_ptr<const ISymbolClass>>>();
    symbolClasses->append(symbolClass);

    Value value;
    value.isDynamic = true;
    value.asDynamicValue.symbolClasses = symbolClasses;
    return value;
}

OsmAnd::IMapStyle::IRuleNode::IRuleNode()
{
}

OsmAnd::IMapStyle::IRuleNode::~IRuleNode()
{
}

OsmAnd::IMapStyle::IRule::IRule()
{
}

OsmAnd::IMapStyle::IRule::~IRule()
{
}

OsmAnd::IMapStyle::IParameter::IParameter()
{
}

OsmAnd::IMapStyle::IParameter::~IParameter()
{
}

OsmAnd::IMapStyle::IAttribute::IAttribute()
{
}

OsmAnd::IMapStyle::IAttribute::~IAttribute()
{
}

OsmAnd::IMapStyle::ISymbolClass::ISymbolClass()
{
}

OsmAnd::IMapStyle::ISymbolClass::~ISymbolClass()
{
}
