#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"

#include "Utilities.h"
#include "Logging.h"

OsmAnd::ObfRoutingSectionInfo::ObfRoutingSectionInfo(const std::weak_ptr<ObfInfo>& owner)
    : ObfSectionInfo(owner)
    , _p(new ObfRoutingSectionInfo_P(this))
    , encodingDecodingRules(_p->_decodingRules)
{
}

OsmAnd::ObfRoutingSectionInfo::~ObfRoutingSectionInfo()
{
}

OsmAnd::ObfRoutingSectionLevel::ObfRoutingSectionLevel(const RoutingDataLevel dataLevel_)
    : _p(new ObfRoutingSectionLevel_P(this))
    , dataLevel(dataLevel_)
    , rootNodes(_p->_rootNodes)
{
}

OsmAnd::ObfRoutingSectionLevel::~ObfRoutingSectionLevel()
{
}

OsmAnd::ObfRoutingSectionLevelTreeNode::ObfRoutingSectionLevelTreeNode()
    : length(0)
    , offset(0)
    , hasChildrenDataBoxes(false)
    , firstDataBoxInnerOffset(0)
    , dataOffset(0)
{
}

OsmAnd::ObfRoutingSectionLevelTreeNode::~ObfRoutingSectionLevelTreeNode()
{
}

OsmAnd::ObfRoutingSectionEncodingDecodingRules::ObfRoutingSectionEncodingDecodingRules()
    : name_encodingRuleId(std::numeric_limits<uint32_t>::max())
{
}

void OsmAnd::ObfRoutingSectionEncodingDecodingRules::createRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue)
{
    auto itEncodingRule = encodingRuleIds.find(ruleTag);
    if (itEncodingRule == encodingRuleIds.end())
        itEncodingRule = encodingRuleIds.insert(ruleTag, QHash<QString, uint32_t>());
    itEncodingRule->insert(ruleValue, ruleId);

    if (!decodingRules.contains(ruleId))
    {
        ObfRoutingSectionDecodingRule rule;
        rule.tag = ruleTag;
        rule.value = ruleValue;

        decodingRules.insert(ruleId, rule);
    }

    if (QLatin1String("name") == ruleTag)
    {
        name_encodingRuleId = ruleId;
        namesRuleId.insert(ruleId);
    }
    else if (ruleTag.startsWith(QLatin1String("name:")))
    {
        const QString languageId = ruleTag.mid(QLatin1String("name:").size());
        localizedName_encodingRuleIds.insert(languageId, ruleId);
        localizedName_decodingRules.insert(ruleId, languageId);
        namesRuleId.insert(ruleId);
    }
    /*else
    {
        LogPrintf(LogSeverityLevel::Debug, "%s = %s", qPrintable(ruleTag), qPrintable(ruleValue));
    }*/
    //if (ruleTag.compare(QLatin1String("oneway"), Qt::CaseInsensitive) == 0)
    //{
    //    rule->type = RuleType::OneWay;
    //    if (ruleValue == QLatin1String("-1") || ruleValue == QLatin1String("reverse"))
    //        rule->parsedValue.asSignedInt = -1;
    //    else if (ruleValue == QLatin1String("1") || ruleValue == QLatin1String("yes"))
    //        rule->parsedValue.asSignedInt = 1;
    //    else
    //        rule->parsedValue.asSignedInt = 0;
    //}
    //else if (ruleTag.compare(QLatin1String("highway"), Qt::CaseInsensitive) == 0 && ruleValue == QLatin1String("traffic_signals"))
    //{
    //    rule->type = RuleType::TrafficSignals;
    //}
    //else if (ruleTag.compare(QLatin1String("railway"), Qt::CaseInsensitive) == 0 && (ruleValue == QLatin1String("crossing") || ruleValue == QLatin1String("level_crossing")))
    //{
    //    rule->type = RuleType::RailwayCrossing;
    //}
    //else if (ruleTag.compare(QLatin1String("roundabout"), Qt::CaseInsensitive) == 0 && !ruleValue.isEmpty())
    //{
    //    rule->type = RuleType::Roundabout;
    //}
    //else if (ruleTag.compare(QLatin1String("junction"), Qt::CaseInsensitive) == 0 && ruleValue.compare(QLatin1String("roundabout"), Qt::CaseInsensitive) == 0)
    //{
    //    rule->type = RuleType::Roundabout;
    //}
    //else if (ruleTag.compare(QLatin1String("highway"), Qt::CaseInsensitive) == 0 && !ruleValue.isEmpty())
    //{
    //    rule->type = RuleType::Highway;
    //}
    //else if (ruleTag.startsWith(QLatin1String("access")) && !ruleValue.isEmpty())
    //{
    //    rule->type = RuleType::Access;
    //}
    //else if (ruleTag.compare(QLatin1String("maxspeed"), Qt::CaseInsensitive) == 0 && !ruleValue.isEmpty())
    //{
    //    rule->type = RuleType::Maxspeed;
    //    rule->parsedValue.asFloat = Utilities::parseSpeed(ruleValue, -1.0);
    //}
    //else if (ruleTag.compare(QLatin1String("lanes"), Qt::CaseInsensitive) == 0 && !ruleValue.isEmpty())
    //{
    //    rule->type = RuleType::Lanes;
    //    rule->parsedValue.asSignedInt = Utilities::parseArbitraryInt(ruleValue, -1);
    //}

    //// Fill gaps in IDs
    //while (decodingRules.size() < rule->id)
    //    decodingRules.push_back(nullptr);
    //decodingRules.push_back(rule);
}

void OsmAnd::ObfRoutingSectionEncodingDecodingRules::createMissingRules()
{
    auto nextId = decodingRules.size() * 2 + 1;

    // Create 'name' encoding/decoding rule, if it still does not exist
    if (name_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        createRule(nextId++,
            QLatin1String("name"), QString::null);
    }
}
