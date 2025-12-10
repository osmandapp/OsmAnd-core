#include "ObfRoutingSectionInfo.h"
#include "ObfRoutingSectionInfo_P.h"

#include "Utilities.h"
#include "Logging.h"

OsmAnd::ObfRoutingSectionInfo::ObfRoutingSectionInfo(const std::shared_ptr<const ObfInfo>& container)
    : ObfSectionInfo(container)
    , _p(new ObfRoutingSectionInfo_P(this))
{
}

OsmAnd::ObfRoutingSectionInfo::~ObfRoutingSectionInfo()
{
}

std::shared_ptr<const OsmAnd::ObfRoutingSectionAttributeMapping> OsmAnd::ObfRoutingSectionInfo::getAttributeMapping() const
{
    return _p->getAttributeMapping();
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

OsmAnd::ObfRoutingSectionAttributeMapping::ObfRoutingSectionAttributeMapping()
{
    decodeMap.reserve(4096);
}

OsmAnd::ObfRoutingSectionAttributeMapping::~ObfRoutingSectionAttributeMapping()
{
}

void OsmAnd::ObfRoutingSectionAttributeMapping::registerMapping(
                                                          const uint32_t id,
                                                          const QString& tag,
                                                          const QString& value)
{
    MapObject::AttributeMapping::registerMapping(id, tag, value);

    // Create decode mapping
    auto pDecode = decodeMap.getRef(id);
    if (!pDecode)
    {
        LogPrintf(LogSeverityLevel::Error,
                  "Decode attribute #%u is not defined. Capture of %s = %s ignored.",
                  id,
                  qPrintable(tag),
                  qPrintable(value));
        return;
    }
    const QStringRef tagRef(&pDecode->tag);
    const QStringRef valueRef(&pDecode->value);
    
    // Create decode mapping
    auto pRoutingDecode = routingDecodeMap.getRef(id);
    if (!pRoutingDecode)
    {
        RouteTypeRule rt(tag, value);
        pRoutingDecode = routingDecodeMap.insert(id, rt);
    }
    
    // Capture quick-access rules
    if (tag.startsWith(QLatin1String("ref:")))
    {
        const auto languageIdRef = tagRef.mid(QLatin1String("ref:").size());
        localizedRefAttributes.insert(languageIdRef, id);
        localizedRefAttributeIds.insert(id, languageIdRef);
        refAttributeIds.insert(id);
    }
}

OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeCondition::RouteTypeCondition() : condition(""), hours(nullptr), floatValue(0)
{
}

OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeCondition::~RouteTypeCondition()
{
}

OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::RouteTypeRule()
: intValue(0), floatValue(0), type(0), forward(0)
{
}

OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::RouteTypeRule(const QString& tag, const QString& value)
: intValue(0), floatValue(0), type(0), forward(0)
{
    t = tag;
    if (QLatin1String("true") == value)
        v = QLatin1String("yes");
    else if (QLatin1String("false") == value)
        v = QLatin1String("no");
    else
        v = value;

    analyze();
}

OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::~RouteTypeRule()
{
}

int OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::isForward() const
{
    return forward;
}

QString OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::getTag() const
{
    return t;
}

QString OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::getValue() const
{
    return v;
}

bool OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::roundabout() const
{
    return type == RouteTypeRule::Roundabout;
}

int OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::getType() const
{
    return type;
}

bool OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::conditional() const
{
    return !conditions.empty();
}

int OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::onewayDirection() const
{
    if (type == RouteTypeRule::OneWay)
        return intValue;
    
    return 0;
}

float OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::maxSpeed() const
{
    if (type == RouteTypeRule::MaxSpeed)
    {
        if (!conditions.empty())
        {
            for (auto& c : conditions)
            {
                if (c->hours && c->hours->isOpened()) {
                    return c->floatValue;
                }
            }
        }
        return floatValue;
    }
    return -1;
}

int OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::lanes() const
{
    if (type == RouteTypeRule::Lanes)
        return intValue;
    
    return -1;
}

QString OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::highwayRoad() const
{
    if (type == RouteTypeRule::HighwayType)
        return v;
    
    return QString();
}

void OsmAnd::ObfRoutingSectionAttributeMapping::RouteTypeRule::analyze()
{
    if (t.compare(QLatin1String("oneway"), Qt::CaseInsensitive) == 0)
    {
        type = RouteTypeRule::OneWay;
        if (QLatin1String("-1") == v || QLatin1String("reverse") == v)
            intValue = -1;
        else if(QLatin1String("1") == v || QLatin1String("yes") == v)
            intValue = 1;
        else
            intValue = 0;
    }
    else if (t.compare(QLatin1String("highway"), Qt::CaseInsensitive) == 0 && QLatin1String("traffic_signals") == v)
    {
        type = RouteTypeRule::TrafficSignals;
    }
    else if (t.compare(QLatin1String("railway"), Qt::CaseInsensitive) == 0 && (QLatin1String("crossing") == v || QLatin1String("level_crossing") == v))
    {
        type = RouteTypeRule::RailwayCrossing;
    }
    else if(t.compare(QLatin1String("roundabout"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        type = RouteTypeRule::Roundabout;
    }
    else if (t.compare(QLatin1String("junction"), Qt::CaseInsensitive) == 0 && QString("roundabout").compare(v, Qt::CaseInsensitive) == 0)
    {
        type = RouteTypeRule::Roundabout;
    }
    else if (t.compare(QLatin1String("highway"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        type = RouteTypeRule::HighwayType;
    }
    else if (t.startsWith(QLatin1String("access")) && !v.isNull())
    {
        type = RouteTypeRule::Access;
    }
    else if (t.compare(QLatin1String("maxspeed:conditional"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        QStringList cts = v.split(';');
        for (const auto& c : cts)
        {
            int ch = c.indexOf('@');
            if (ch > 0)
            {
                const auto cond = std::make_shared<RouteTypeCondition>();
                cond->floatValue = Utilities::parseSpeed(c.mid(0, ch), 0);
                cond->condition = c.mid(0, ch + 1).trimmed();
                if (cond->condition.startsWith('('))
                    cond->condition = cond->condition.mid(1).trimmed();
                
                if (cond->condition.endsWith(')'))
                    cond->condition = cond->condition.mid(0, cond->condition.length() - 1).trimmed();
                
                cond->hours = OpeningHoursParser::parseOpenedHours(cond->condition.toStdString());
                conditions.push_back(cond);
            }
        }
        type = RouteTypeRule::MaxSpeed;
    }
    else if (t.compare(QLatin1String("maxspeed"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        type = RouteTypeRule::MaxSpeed;
        floatValue = Utilities::parseSpeed(v, 0);
    }
    else if (t.compare(QLatin1String("maxspeed:forward"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        type = RouteTypeRule::MaxSpeed;
        forward = 1;
        floatValue = Utilities::parseSpeed(v, 0);
    }
    else if(t.compare(QLatin1String("maxspeed:backward"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        type = RouteTypeRule::MaxSpeed;
        forward = -1;
        floatValue = Utilities::parseSpeed(v, 0);
    }
    else if (t.compare(QLatin1String("lanes"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        type = RouteTypeRule::Lanes;
        intValue = Utilities::parseArbitraryInt(v, -1);
    }
    else if (t.compare(QLatin1String("hazard"), Qt::CaseInsensitive) == 0 && !v.isNull())
    {
        type = RouteTypeRule::Hazard;
    }
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
