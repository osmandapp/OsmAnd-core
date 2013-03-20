#include "RoutingRulesetContext.h"

OsmAnd::RoutingRulesetContext::RoutingRulesetContext()
{
}

OsmAnd::RoutingRulesetContext::~RoutingRulesetContext()
{
}

void OsmAnd::RoutingRulesetContext::registerSelectExpression( const QString& value, const QString& type )
{
    std::shared_ptr<RoutingRuleExpression> expression(new RoutingRuleExpression(value, type));
    _expressions.push_back(expression);
}
