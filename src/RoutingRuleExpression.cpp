#include "RoutingRuleExpression.h"
#include "RoutingRuleExpression_Operators.h"

#include <OsmAndCommon.h>
#include <OsmAndLogging.h>
#include <RoutingConfiguration.h>
#include <RoutingRuleset.h>
#include <RoutingProfile.h>

OsmAnd::RoutingRuleExpression::RoutingRuleExpression( RoutingRuleset* ruleset, const QString& value, const QString& type )
    : ruleset(ruleset)
    , _type(type)
    , parameters(_parameters)
    , type(_type)
{
    if(value.startsWith(":"))
        _variableRef = value.mid(1);
    else if (value.startsWith("$"))
        _tagRef = value.mid(1);
    else
    {
        const auto wasParsed = RoutingConfiguration::parseTypedValue(value.trimmed(), type, _value);
        OSMAND_ASSERT(wasParsed, "Value '" << qPrintable(value) << "' can not be parsed");
    }
}

OsmAnd::RoutingRuleExpression::~RoutingRuleExpression()
{
}

void OsmAnd::RoutingRuleExpression::registerAndTagValue( const QString& tag, const QString& value, bool negation )
{
    if(value.isEmpty())
    {
        if(negation)
            _onlyNotTags.insert(tag);
        else
            _onlyTags.insert(tag);
    }
    else
    {
        auto valueType = ruleset->owner->registerTagValueAttribute(tag, value);

        if(_filterNotTypes.size() <= valueType)
            _filterNotTypes.resize(valueType + 1);
        if(_filterTypes.size() <= valueType)
            _filterTypes.resize(valueType + 1);

        if(negation)
            _filterNotTypes.setBit(valueType);
        else
            _filterTypes.setBit(valueType);
    }
}

void OsmAnd::RoutingRuleExpression::registerLessCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<Operator>(new Operator_L(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerLessOrEqualCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<Operator>(new Operator_LE(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerGreaterCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<Operator>(new Operator_G(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerGreaterOrEqualCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<Operator>(new Operator_GE(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerAndParamCondition( const QString& param, bool negation )
{
    _parameters.push_back(negation ? "-" + param : param);
}

bool OsmAnd::RoutingRuleExpression::evaluate( const QBitArray& types, RoutingRulesetContext* context, ResultType resultType, void* result ) const
{
    if(!validate(types, context))
        return false;

    if(!evaluateExpressions(types, context))
        return false;

    float value;
    bool ok = false;
    if(!_tagRef.isEmpty())
        ok = resolveTagReferenceValue(context, types, _tagRef, type, value);
    else if(!_variableRef.isEmpty())
        ok = resolveVariableReferenceValue(context, _variableRef, type, value);
    else
    {
        value = _value;
        ok = true;
    }
    
    if(ok)
    {
        if(resultType == Float)
            *reinterpret_cast<float*>(result) = value;
        else if(resultType == Integer)
            *reinterpret_cast<int*>(result) = (int)value;
        else
            ok = false;
    }
    return ok;
}

bool OsmAnd::RoutingRuleExpression::validate( const QBitArray& types, RoutingRulesetContext* context ) const
{
    if(!validateAllTypesShouldBePresent(types))
        return false;
    
    if(!validateAllTypesShouldNotBePresent(types))
        return false;
    
    if(!validateFreeTags(types))
        return false;
    
    if(!validateNotFreeTags(types))
        return false;
    
    return true;
}

bool OsmAnd::RoutingRuleExpression::validateAllTypesShouldBePresent( const QBitArray& types ) const
{
    const auto& intermediate = types & _filterTypes;
    return intermediate.count(true) == _filterTypes.count(true);
}

bool OsmAnd::RoutingRuleExpression::validateAllTypesShouldNotBePresent( const QBitArray& types ) const
{
    if(_filterNotTypes.isEmpty())
        return true;

    const auto& intermediate = types & _filterNotTypes;
    return intermediate.count(true) == 0;
}

bool OsmAnd::RoutingRuleExpression::validateFreeTags( const QBitArray& types ) const
{
    for(auto itOnlyTag = _onlyTags.begin(); itOnlyTag != _onlyTags.end(); ++itOnlyTag)
    {
        auto itBitset = ruleset->owner->_tagRuleMask.find(*itOnlyTag);
        if(itBitset == ruleset->owner->_tagRuleMask.end())
            return false;
        if( (*itBitset & types).count(true) == 0 )
            return false;
    }
    
    return true;
}

bool OsmAnd::RoutingRuleExpression::validateNotFreeTags( const QBitArray& types ) const
{
    for(auto itOnlyNotTag = _onlyNotTags.begin(); itOnlyNotTag != _onlyNotTags.end(); ++itOnlyNotTag)
    {
        auto itBitset = ruleset->owner->_tagRuleMask.find(*itOnlyNotTag);
        if(itBitset == ruleset->owner->_tagRuleMask.end())
            return false;
        if( (*itBitset & types).count(true) > 0 )
            return false;
    }

    return true;
}

bool OsmAnd::RoutingRuleExpression::evaluateExpressions( const QBitArray& types, RoutingRulesetContext* context ) const
{
    for(auto itOperator = _operators.begin(); itOperator != _operators.end(); ++itOperator)
    {
        auto operator_ = *itOperator;
        if(!operator_->evaluate(types, context))
            return false;
    }

    return true;
}

bool OsmAnd::RoutingRuleExpression::resolveVariableReferenceValue( RoutingRulesetContext* context, const QString& variableRef, const QString& type, float& value )
{
    bool ok = false;
    auto itVariable = context->contextValues.find(variableRef);
    if(itVariable != context->contextValues.end())
        ok = RoutingConfiguration::parseTypedValue(itVariable.value(), type, value);
    return ok;
}

bool OsmAnd::RoutingRuleExpression::resolveTagReferenceValue( RoutingRulesetContext* context, const QBitArray& types, const QString& tagRef, const QString& type, float& value )
{
    bool ok = false;
    auto itMask = context->ruleset->owner->_tagRuleMask.find(tagRef);
    if(itMask != context->ruleset->owner->_tagRuleMask.end())
    {
        const auto& mask = *itMask;
        auto foundBits = (mask & types);
        if(foundBits.count(true) > 0)
        {
            for(auto bitIdx = 0; bitIdx < foundBits.size(); bitIdx++)
            {
                if(foundBits.testBit(bitIdx))
                {
                    ok = context->ruleset->owner->parseTypedValueFromTag(bitIdx, type, value);
                    break;
                }
            }
        }
    }

    return ok;
}

OsmAnd::RoutingRuleExpression::Operator::Operator()
{
}

OsmAnd::RoutingRuleExpression::Operator::~Operator()
{

}
