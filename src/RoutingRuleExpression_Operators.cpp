#include "RoutingRuleExpression_Operators.h"

OsmAnd::RoutingRuleExpressionOperator::RoutingRuleExpressionOperator()
{

}

OsmAnd::RoutingRuleExpressionOperator::~RoutingRuleExpressionOperator()
{

}


OsmAnd::RoutingRuleExpressionBinaryOperator::RoutingRuleExpressionBinaryOperator( const QString& lvalue, const QString& rvalue, const QString& type )
{

}

OsmAnd::RoutingRuleExpressionBinaryOperator::~RoutingRuleExpressionBinaryOperator()
{

}

OsmAnd::Operator_G::Operator_G( const QString& lvalue, const QString& rvalue, const QString& type )
    : RoutingRuleExpressionBinaryOperator(lvalue, rvalue, type)
{

}

OsmAnd::Operator_G::~Operator_G()
{

}

OsmAnd::Operator_GE::Operator_GE( const QString& lvalue, const QString& rvalue, const QString& type )
    : RoutingRuleExpressionBinaryOperator(lvalue, rvalue, type)
{

}

OsmAnd::Operator_GE::~Operator_GE()
{

}

OsmAnd::Operator_L::Operator_L( const QString& lvalue, const QString& rvalue, const QString& type )
    : RoutingRuleExpressionBinaryOperator(lvalue, rvalue, type)
{

}

OsmAnd::Operator_L::~Operator_L()
{

}

OsmAnd::Operator_LE::Operator_LE( const QString& lvalue, const QString& rvalue, const QString& type )
    : RoutingRuleExpressionBinaryOperator(lvalue, rvalue, type)
{

}

OsmAnd::Operator_LE::~Operator_LE()
{

}
