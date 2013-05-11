#include "RoutingConfiguration.h"
#include "RoutingConfiguration_private.h"

#include "Common.h"
#include "EmbeddedResources.h"
#include "Utilities.h"

#include <QByteArray>
#include <QBuffer>
#include <QXmlStreamReader>
#include <QStringList>

OsmAnd::RoutingConfiguration::RoutingConfiguration()
    : routingProfiles(_routingProfiles)
{
}

OsmAnd::RoutingConfiguration::~RoutingConfiguration()
{
}

bool OsmAnd::RoutingConfiguration::parseConfiguration( QIODevice* data, RoutingConfiguration& outConfig )
{
    QXmlStreamReader xmlReader(data);

    std::shared_ptr<RoutingProfile> routingProfile;
    auto rulesetType = RoutingRuleset::Type::Invalid;
    QStack< std::shared_ptr<RoutingRule> > ruleset;
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (tagName == "osmand_routing_config")
            {
                outConfig._defaultRoutingProfileName = xmlReader.attributes().value("defaultProfile").toString();
            }
            else if (tagName == "routingProfile")
            {
                routingProfile.reset(new RoutingProfile());
                parseRoutingProfile(&xmlReader, routingProfile.get());
                if(routingProfile->_name == outConfig._defaultRoutingProfileName)
                    outConfig._defaultRoutingProfile = routingProfile;
                outConfig._routingProfiles.insert(routingProfile->_name, routingProfile);
            }
            else if (tagName == "attribute")
            {
                auto name = xmlReader.attributes().value("name").toString();
                auto value = xmlReader.attributes().value("value").toString();
                if(routingProfile)
                    routingProfile->addAttribute(name, value);
                else
                    outConfig._attributes.insert(name, value);
            }
            else if (tagName == "parameter")
            {
                if(!routingProfile)
                {
                    std::cerr << "<parameter> is not inside <routingProfile>" << "(" << xmlReader.lineNumber() << ", " << xmlReader.columnNumber() << " @ " << data->pos() << ")" << std::endl;
                    return false;
                }
                parseRoutingParameter(&xmlReader, routingProfile.get());
            }
            else if (tagName == "point" || tagName == "way")
            {
                assert(rulesetType == RoutingRuleset::Type::Invalid);
                auto attributeName = xmlReader.attributes().value("attribute");
                if (attributeName == "priority")
                    rulesetType = RoutingRuleset::Type::RoadPriorities;
                else if (attributeName == "speed")
                    rulesetType = RoutingRuleset::Type::RoadSpeed;
                else if (attributeName == "access")
                    rulesetType = RoutingRuleset::Type::Access;
                else if (attributeName == "obstacle_time")
                    rulesetType = RoutingRuleset::Type::Obstacles;
                else if (attributeName == "obstacle")
                    rulesetType = RoutingRuleset::Type::RoutingObstacles;
                else if (attributeName == "oneway")
                    rulesetType = RoutingRuleset::Type::OneWay;

                OSMAND_ASSERT(rulesetType != RoutingRuleset::Type::Invalid, "Route data object attribute is unknown " << qPrintable(attributeName.toString()));
            }
            else if(rulesetType != RoutingRuleset::Type::Invalid)
            {
                if(isConditionTag(tagName))
                    parseRoutingRuleset(&xmlReader, routingProfile.get(), rulesetType, ruleset);
            }
        }
        else if (xmlReader.isEndElement())
        {
            if(tagName == "routingProfile")
            {
                routingProfile.reset();
            }
            else if (tagName == "point" || tagName == "way")
            {
                rulesetType = RoutingRuleset::Type::Invalid;
            }
            else if(isConditionTag(tagName))
            {
                ruleset.pop();
            }
        }
    }
    if(xmlReader.hasError())
    {
        std::cerr << qPrintable(xmlReader.errorString()) << "(" << xmlReader.lineNumber() << ", " << xmlReader.columnNumber() << " @ " << data->pos() << ")" << std::endl;
        return false;
    }

    return true;
}

void OsmAnd::RoutingConfiguration::parseRoutingParameter( QXmlStreamReader* xmlParser, RoutingProfile* routingProfile )
{
    const auto& attribs = xmlParser->attributes();
    auto descriptionAttrib = attribs.value("description");
    auto nameAttrib = attribs.value("name");
    auto idAttrib = attribs.value("id");
    auto typeAttrib = attribs.value("type");

    if(typeAttrib == "boolean")
    {
        routingProfile->registerBooleanParameter(idAttrib.toString(), nameAttrib.toString(), descriptionAttrib.toString());
    }
    else if(typeAttrib == "numeric")
    {
        auto combinedValues = attribs.value("values");
        auto valueDescriptions = attribs.value("valueDescriptions");
        const auto& stringifiedValues = combinedValues.toString().split(',');
        QList<double> values;
        for(auto itValue = stringifiedValues.begin(); itValue != stringifiedValues.end(); ++itValue)
        {
            bool ok;
            auto value = itValue->trimmed().toDouble(&ok);
            if(!ok)
                std::cerr << qPrintable(*itValue) << " is not a valid integer" << std::endl;
            values.push_back(value);
        }

        routingProfile->registerNumericParameter(idAttrib.toString(), nameAttrib.toString(), descriptionAttrib.toString(), values, valueDescriptions.string()->split(','));
    }
    else
    {
        OSMAND_ASSERT(0, "Unsupported routing parameter type - " << qPrintable(typeAttrib.toString()));
    }
}

void OsmAnd::RoutingConfiguration::loadDefault( RoutingConfiguration& outConfig )
{
    auto rawDefaultConfig = EmbeddedResources::decompressResource("routing.xml");
    QBuffer defaultConfig(&rawDefaultConfig);
    bool ok = false;
    ok = defaultConfig.open(QIODevice::ReadOnly | QIODevice::Text);
    assert(ok);
    ok = parseConfiguration(&defaultConfig, outConfig);
    assert(ok);
    defaultConfig.close();
}

void OsmAnd::RoutingConfiguration::parseRoutingProfile( QXmlStreamReader* xmlParser, RoutingProfile* routingProfile )
{
    const auto& attribs = xmlParser->attributes();
    routingProfile->_name = attribs.value("name").toString();
    for (auto itAttrib = attribs.begin(); itAttrib != attribs.end(); ++itAttrib)
    {
        const auto& attrib = *itAttrib;
        auto name = attrib.name();
        auto value = attrib.value();
        if(name.isNull())
            continue;
        routingProfile->addAttribute(name.toString(), value.isNull() ? QString() : value.toString());
    }

    /*TODO: Not yet used
    RoutingProfile::Preset baseProfile;
    auto baseProfileAttrib = xmlParser->attributes().value("baseProfile");
    if(baseProfileAttrib == "pedestrian")
        baseProfile = RoutingProfile::Preset::Pedestrian;
    else if(baseProfileAttrib == "bicycle")
        baseProfile = RoutingProfile::Preset::Bicycle;
    else if(baseProfileAttrib == "car")
        baseProfile = RoutingProfile::Preset::Car;
    else
    {
        std::cerr << "Bad baseProfile " << qPrintable(baseProfileAttrib.toString()) << std::endl;
        return false;
    }
    */
}

void OsmAnd::RoutingConfiguration::parseRoutingRuleset( QXmlStreamReader* xmlParser, RoutingProfile* routingProfile, RoutingRuleset::Type rulesetType, QStack< std::shared_ptr<struct RoutingRule> >& ruleset )
{
    const auto& attribs = xmlParser->attributes();

    std::shared_ptr<RoutingRule> routingRule(new RoutingRule());
    routingRule->_tagName = xmlParser->name().toString();
    routingRule->_t = attribs.value("t").toString();
    routingRule->_v = attribs.value("v").toString();
    routingRule->_param = attribs.value("param").toString();
    routingRule->_value1 = attribs.value("value1").toString();
    routingRule->_value2 = attribs.value("value2").toString();
    routingRule->_type = attribs.value("type").toString();

    auto context = routingProfile->getRuleset(rulesetType);
    if(routingRule->_tagName == "select")
    {
        context->registerSelectExpression(attribs.value("value").toString(), attribs.value("type").toString());
        addRulesetSubclause(routingRule.get(), context.get());

        for(auto itItem = ruleset.begin(); itItem != ruleset.end(); ++itItem)
            addRulesetSubclause((*itItem).get(), context.get());
    }
    else if (!ruleset.isEmpty() && ruleset.top()->_tagName == "select")
    {
        addRulesetSubclause(routingRule.get(), context.get());
    }

    ruleset.push(routingRule);
}

void OsmAnd::RoutingConfiguration::addRulesetSubclause( RoutingRule* routingRule, RoutingRuleset* ruleset )
{
    auto lastExpression = ruleset->_expressions.last();

    if (!routingRule->_param.isEmpty())
        lastExpression->registerAndParamCondition(routingRule->_param, (routingRule->_tagName == "ifnot"));

    if (!routingRule->_t.isEmpty())
        lastExpression->registerAndTagValue(routingRule->_t, routingRule->_v, (routingRule->_tagName == "ifnot"));

    if (routingRule->_tagName == "ge")
        lastExpression->registerGreaterOrEqualCondition(routingRule->_value1, routingRule->_value2, routingRule->_type);
    else if (routingRule->_tagName == "g")
        lastExpression->registerGreaterCondition(routingRule->_value1, routingRule->_value2, routingRule->_type);
    else if (routingRule->_tagName == "le")
        lastExpression->registerLessOrEqualCondition(routingRule->_value1, routingRule->_value2, routingRule->_type);
    else if (routingRule->_tagName == "l")
        lastExpression->registerLessCondition(routingRule->_value1, routingRule->_value2, routingRule->_type);
}

bool OsmAnd::RoutingConfiguration::isConditionTag( const QStringRef& tagName )
{
    return tagName == "select" || tagName == "if" || tagName == "ifnot" || tagName == "ge" || tagName == "le" || tagName == "g" || tagName == "l";
}

QString OsmAnd::RoutingConfiguration::resolveAttribute( const QString& vehicle, const QString& name )
{
    auto itProfile = _routingProfiles.find(vehicle);
    if(itProfile != _routingProfiles.end())
    {
        auto itProfileAttribute = (*itProfile)->_attributes.find(name);
        if(itProfileAttribute != (*itProfile)->_attributes.end())
            return (*itProfileAttribute);
    }

    auto itAttribute = _attributes.find(name);
    if(itAttribute != _attributes.end())
        return *itAttribute;
    return QString();
}

bool OsmAnd::RoutingConfiguration::parseTypedValue( const QString& value, const QString& type, float& parsedValue )
{
    parsedValue;
    bool ok;
    
    if(type == "speed")
        parsedValue = Utilities::parseSpeed(value, 0, &ok);
    else if(type == "weight")
        parsedValue = Utilities::parseWeight(value, 0, &ok);
    else if(type == "length")
        parsedValue = Utilities::parseLength(value, 0, &ok);
    else
        parsedValue = Utilities::parseArbitraryFloat(value, 0, &ok);
    
    return ok;
}
