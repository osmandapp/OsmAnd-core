#include "RoutingRulesetContext.h"

#include <RoutingProfile.h>
#include <RoutingProfileContext.h>

OsmAnd::RoutingRulesetContext::RoutingRulesetContext(RoutingProfileContext* owner_, std::shared_ptr<RoutingRuleset> ruleset_, QHash<QString, QString>* contextValues_)
    : ruleset(ruleset_)
    , owner(owner_)
    , contextValues(_contextValues)
{
    if(contextValues_)
        _contextValues = *contextValues_;
}

OsmAnd::RoutingRulesetContext::~RoutingRulesetContext()
{
}

int OsmAnd::RoutingRulesetContext::evaluateAsInteger( Model::Road* road, int defaultValue )
{
    int result;
    if(!evaluate(road, RoutingRuleExpression::ResultType::Integer, &result))
        return defaultValue;
    return result;
}

float OsmAnd::RoutingRulesetContext::evaluateAsFloat( Model::Road* road, float defaultValue )
{
    float result;
    if(!evaluate(road, RoutingRuleExpression::ResultType::Float, &result))
        return defaultValue;
    return result;
}

int OsmAnd::RoutingRulesetContext::evaluateAsInteger( ObfRoutingSection* section, const QVector<uint32_t>& roadTypes, int defaultValue )
{
    int result;
    if(!evaluate(encode(section, roadTypes), RoutingRuleExpression::ResultType::Integer, &result))
        return defaultValue;
    return result;
}

float OsmAnd::RoutingRulesetContext::evaluateAsFloat( ObfRoutingSection* section, const QVector<uint32_t>& roadTypes, float defaultValue )
{
    float result;
    if(!evaluate(encode(section, roadTypes), RoutingRuleExpression::ResultType::Float, &result))
        return defaultValue;
    return result;
}

bool OsmAnd::RoutingRulesetContext::evaluate( Model::Road* road, RoutingRuleExpression::ResultType type, void* result )
{
    return evaluate(encode(road->subsection->section.get(), road->types), type, result);
}

bool OsmAnd::RoutingRulesetContext::evaluate( const QBitArray& types, RoutingRuleExpression::ResultType type, void* result )
{
    uint32_t ruleIdx = 0;
    for(auto itExpression = ruleset->expressions.begin(); itExpression != ruleset->expressions.end(); ++itExpression, ruleIdx++)
    {
        auto expression = *itExpression;
        if(expression->evaluate(types, this, type, result))
            return true;
    }
    return false;
}

QBitArray OsmAnd::RoutingRulesetContext::encode( ObfRoutingSection* section, const QVector<uint32_t>& roadTypes )
{
    QBitArray bitset(ruleset->owner->_universalRules.size());
    
    auto itTagValueAttribIdCache = owner->_tagValueAttribIdCache.find(section);
    if(itTagValueAttribIdCache == owner->_tagValueAttribIdCache.end())
        itTagValueAttribIdCache = owner->_tagValueAttribIdCache.insert(section, QMap<uint32_t, uint32_t>());
    
    for(auto itType = roadTypes.begin(); itType != roadTypes.end(); ++itType)
    {
        auto type = *itType;

        auto itId = itTagValueAttribIdCache->find(type);
        if(itId == itTagValueAttribIdCache->end())
        {
            auto encodingRule = section->_encodingRules[type];
            assert(encodingRule);

            auto id = ruleset->owner->registerTagValueAttribute(encodingRule->_tag, encodingRule->_value);
            itId = itTagValueAttribIdCache->insert(type, id);
        }
        auto id = *itId;

        if(bitset.size() <= id)
            bitset.resize(id + 1);
        bitset.setBit(id);
    }

    return bitset;
}
