#include "RoutingRuleExpression.h"
#include "RoutingRuleExpression_Operators.h"

OsmAnd::RoutingRuleExpression::RoutingRuleExpression( const QString& value, const QString& type )
{
    /*if(value.startsWith(":") || value.startsWith("$")) {
        selectValue = value;
    } else {
        selectValue = parseValue(value, type);
        if(selectValue == null) {
            System.err.println("Routing.xml select value '" + value+"' was not registered");
        }
    }*/
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
        /*int vtype = registerTagValueAttribute(tag, value);
        if(not) {
            filterNotTypes.set(vtype);
        } else {
            filterTypes.set(vtype);
        }*/
    }
}

void OsmAnd::RoutingRuleExpression::registerLessCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<RoutingRuleExpressionOperator>(new Operator_L(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerLessOrEqualCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<RoutingRuleExpressionOperator>(new Operator_LE(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerGreaterCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<RoutingRuleExpressionOperator>(new Operator_G(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerGreaterOrEqualCondition( const QString& lvalue, const QString& rvalue, const QString& type )
{
    _operators.push_back(std::shared_ptr<RoutingRuleExpressionOperator>(new Operator_GE(lvalue, rvalue, type)));
}

void OsmAnd::RoutingRuleExpression::registerAndParamCondition( const QString& param, bool negation )
{
    _parameters.push_back(negation ? "-" + param : param);
}
