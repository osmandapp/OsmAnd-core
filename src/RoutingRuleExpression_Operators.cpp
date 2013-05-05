#include "RoutingRuleExpression_Operators.h"

#include "Common.h"
#include "RoutingConfiguration.h"

OsmAnd::BinaryOperator::BinaryOperator(const QString& lvalue, const QString& rvalue, const QString& type)
    : type(type)
{
    if(lvalue.startsWith(":"))
        _lVariableRef = lvalue.mid(1);
    else if (lvalue.startsWith("$"))
        _lTagRef = lvalue.mid(1);
    else
    {
        const auto wasParsed = RoutingConfiguration::parseTypedValue(lvalue.trimmed(), type, _lValue);
        OSMAND_ASSERT(wasParsed, "LValue '" << qPrintable(lvalue) << "' can not be parsed");
    }

    if(rvalue.startsWith(":"))
        _rVariableRef = rvalue.mid(1);
    else if (rvalue.startsWith("$"))
        _rTagRef = rvalue.mid(1);
    else
    {
        const auto wasParsed = RoutingConfiguration::parseTypedValue(rvalue.trimmed(), type, _rValue);
        OSMAND_ASSERT(wasParsed, "RValue '" << qPrintable(rvalue) << "' can not be parsed");
    }
}

OsmAnd::BinaryOperator::~BinaryOperator()
{
}

bool OsmAnd::BinaryOperator::evaluate( const QBitArray& types, RoutingRulesetContext* context ) const
{
    bool ok = false;

    float lValue;
    if(!_lTagRef.isEmpty())
        ok = RoutingRuleExpression::resolveTagReferenceValue(context, types, _lTagRef, type, lValue);
    else if(!_lVariableRef.isEmpty())
        ok = RoutingRuleExpression::resolveVariableReferenceValue(context, _lVariableRef, type, lValue);
    else
    {
        lValue = _lValue;
        ok = true;
    }
    if(!ok)
        return false;
    
    ok = false;
    float rValue;
    if(!_rTagRef.isEmpty())
        ok = RoutingRuleExpression::resolveTagReferenceValue(context, types, _rTagRef, type, rValue);
    else if(!_rVariableRef.isEmpty())
        ok = RoutingRuleExpression::resolveVariableReferenceValue(context, _rVariableRef, type, rValue);
    else
    {
        rValue = _rValue;
        ok = true;
    }
    if(!ok)
        return false;
    
    return evaluateValues(lValue, rValue);
}

OsmAnd::Operator_G::Operator_G( const QString& lvalue, const QString& rvalue, const QString& type )
    : BinaryOperator(lvalue, rvalue, type)
{
}

OsmAnd::Operator_G::~Operator_G()
{
}

bool OsmAnd::Operator_G::evaluateValues( float lValue, float rValue ) const
{
    return lValue > rValue;
}

OsmAnd::Operator_GE::Operator_GE( const QString& lvalue, const QString& rvalue, const QString& type )
    : BinaryOperator(lvalue, rvalue, type)
{
}

OsmAnd::Operator_GE::~Operator_GE()
{
}

bool OsmAnd::Operator_GE::evaluateValues( float lValue, float rValue ) const
{
    return lValue >= rValue;
}

OsmAnd::Operator_L::Operator_L( const QString& lvalue, const QString& rvalue, const QString& type )
    : BinaryOperator(lvalue, rvalue, type)
{
}

OsmAnd::Operator_L::~Operator_L()
{
}

bool OsmAnd::Operator_L::evaluateValues( float lValue, float rValue ) const
{
    return lValue < rValue;
}

OsmAnd::Operator_LE::Operator_LE( const QString& lvalue, const QString& rvalue, const QString& type )
    : BinaryOperator(lvalue, rvalue, type)
{
}

OsmAnd::Operator_LE::~Operator_LE()
{
}

bool OsmAnd::Operator_LE::evaluateValues( float lValue, float rValue ) const
{
    return lValue <= rValue;
}
