#include "RoutingRuleset.h"

OsmAnd::RoutingRuleset::RoutingRuleset( RoutingProfile* owner, Type type )
    : owner(owner)
    , type(type)
    , expressions(_expressions)
{
}

OsmAnd::RoutingRuleset::~RoutingRuleset()
{
}

void OsmAnd::RoutingRuleset::registerSelectExpression( const QString& value, const QString& type )
{
    const std::shared_ptr<RoutingRuleExpression> expression(new RoutingRuleExpression(this, value, type));
    _expressions.push_back(qMove(expression));
}
