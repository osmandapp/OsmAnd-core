#include "RoutingRulesetContext.h"

#include <RoutingProfile.h>
#include <RoutingProfileContext.h>

bool checkParameter(std::shared_ptr<OsmAnd::RoutingRuleExpression> rt,
                    const QHash<QString, QString>& contextValues_) {

    for (QString p : rt->parameters) {
        bool nott = false;
        if (p.startsWith("-")) {
            nott = true;
            p = p.right(-1);
        }
        bool val = contextValues_.contains(p);
        if (nott && val) {
            return false;
        } else if (!nott && !val) {
            return false;
        }
    }
    return true;

}

OsmAnd::RoutingRulesetContext::RoutingRulesetContext(RoutingProfileContext* owner_, const std::shared_ptr<RoutingRuleset>& ruleset_, QHash<QString, QString>* contextValues_)
    : owner(owner_)
    ,_ruleset(new RoutingRuleset(ruleset_->owner, ruleset_->type))
    , ruleset(_ruleset)
    , contextValues(_contextValues)
{
    if(contextValues_)
        _contextValues = *contextValues_;

    for(std::shared_ptr<RoutingRuleExpression> rt : ruleset_->_expressions){
        if(checkParameter(rt, _contextValues)){
            _ruleset->_expressions.push_back(rt);
        }
    }
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
    //for(std::shared_ptr<RoutingRuleExpression> itExpression = ruleset->expressions.begin(); itExpression != ruleset->expressions.end(); ++itExpression)
    for(std::shared_ptr<RoutingRuleExpression>  expression  : ruleset->expressions)
    {
//        auto expression = *itExpression;
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
