#include "RoutingConfiguration.h"

#include "Common.h"

#include <QByteArray>
#include <QBuffer>
#include <QXmlStreamReader>
#include <QStringList>

OsmAnd::RoutingConfiguration::RoutingConfiguration()
{
}

OsmAnd::RoutingConfiguration::~RoutingConfiguration()
{
}

bool OsmAnd::RoutingConfiguration::parseConfiguration( QIODevice* data, RoutingConfiguration& outConfig )
{
    QXmlStreamReader xmlReader(data);
    
    //std::shared_ptr<RoutingVehicleConfig> currentRouter;
    //QStack<RoutingRule > rulesStack;
    //RouteDataObjectAttribute currentAttribute;

    std::shared_ptr<RoutingProfile> routingProfile;
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
                parseRoutingProfile(&xmlReader, routingProfile);
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
                parseRoutingParameter(&xmlReader, routingProfile);
            }
            else if (tagName == "point" || tagName == "way")
            {
                /*QString attribute = parser.attributes().
                    value( "attribute").toString();
                currentAttribute = valueOfRouteDataObjectAttribute(attribute);*/
            }
            else
            {
                //parseRoutingRule(parser, currentRouter, currentAttribute, rulesStack);
            }
        }
        else if (xmlReader.isEndElement())
        {
            if(tagName == "routingProfile")
            {
                routingProfile.reset();
            }
            /*if (checkTag(pname))
            {
                rulesStack.pop();
            }*/
        }
    }
    if(xmlReader.hasError())
    {
        std::cerr << qPrintable(xmlReader.errorString()) << "(" << xmlReader.lineNumber() << ", " << xmlReader.columnNumber() << " @ " << data->pos() << ")" << std::endl;
        return false;
    }
    return true;
}

void OsmAnd::RoutingConfiguration::parseRoutingParameter( QXmlStreamReader* xmlParser, std::shared_ptr<RoutingProfile>& outRoutingProfile )
{
    const auto& attribs = xmlParser->attributes();
    auto descriptionAttrib = attribs.value("description");
    auto nameAttrib = attribs.value("name");
    auto idAttrib = attribs.value("id");
    auto typeAttrib = attribs.value("type");

    if(typeAttrib.compare("boolean", Qt::CaseInsensitive) == 0)
    {
        //TODO:outRoutingProfile->registerBooleanParameter(idAttrib.toString(), nameAttrib.toString(), descriptionAttrib.toString());
    }
    else if(typeAttrib == "numeric")
    {
        auto combinedValues = attribs.value("values");
        auto valueDescriptions = attribs.value("valueDescriptions");
        const auto& stringifiedValues = combinedValues.string()->split(',');
        QList<double> values;
        for(auto itValue = stringifiedValues.begin(); itValue != stringifiedValues.end(); ++itValue)
        {
            bool ok;
            auto value = itValue->trimmed().toDouble(&ok);
            if(!ok)
                std::cerr << qPrintable(*itValue) << " is not a valid integer" << std::endl;
            values.push_back(value);
        }
            
        /*TODO:currentRouter->registerNumericParameter(id, name, description, vls ,
            valueDescriptions.split(","));*/
    }
    else
    {
        OSMAND_ASSERT(0, "Unsupported routing parameter type - " << qPrintable(typeAttrib.toString()));
    }
}

void OsmAnd::RoutingConfiguration::loadDefault( RoutingConfiguration& outConfig )
{
    QByteArray rawDefaultConfig(reinterpret_cast<const char*>(&_defaultRawXml[0]), _defaultRawXmlSize);
    QBuffer defaultConfig(&rawDefaultConfig);
    assert(defaultConfig.open(QIODevice::ReadOnly | QIODevice::Text));
    assert(parseConfiguration(&defaultConfig, outConfig));
    defaultConfig.close();
}

void OsmAnd::RoutingConfiguration::parseRoutingProfile( QXmlStreamReader* xmlParser, std::shared_ptr<RoutingProfile>& outRoutingProfile )
{
    const auto& attribs = xmlParser->attributes();
    outRoutingProfile->_name = attribs.value("name").toString();
    for (auto itAttrib = attribs.begin(); itAttrib != attribs.end(); ++itAttrib)
    {
        const auto& attrib = *itAttrib;
        auto name = attrib.name();
        auto value = attrib.value();
        if(name.isNull())
            continue;
        outRoutingProfile->addAttribute(name.toString(), value.isNull() ? QString() : value.toString());
    }

    //TODO: Override? config.defaultRouterName = parser.attributes().value("name").toString();
    /*Useless?
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



//#include "RoutingVehicleConfig.h"
//#include <qxmlstream.h>
//#include <qstack.h>
//#include <qstringlist.h>
//
//
//
//namespace OsmAnd {
//QString RoutingConfiguration::getAttribute(RoutingVehicleConfig& router, QString& propertyName) {
//	if (router.containsAttribute(propertyName)) {
//		return router.getAttribute(propertyName);
//	}
//	return attributes[propertyName];
//}
//
///*
//RoutingConfiguration::RoutingConfiguration(shared_ptr<RoutingConfigurationFile> file, QString rt, double direction = -720,
//			int memoryLimitMB = 30, QMap<QString, QString>& params){
//		routerName = file->routers.contains(rt) ? rt : file->defaultRouterName;
//		router = file->routers[routerName];
//		if (!params.isEmpty()) {
//			this->router = this->router.build(params);
//		}
//		attributes["routerName"] = router;
//		QString t = this->getAttribute(this->router, rt);
//		attributes.putAll(file->attributes);
//		recalculateDistance = parseSilentFloat(getAttribute(router, "recalculateDistanceHelp"),
//				10000);
//		heuristicCoefficient = parseSilentFloat(getAttribute(router, "heuristicCoefficient"),
//				1);
//		zoomToLoadTiles = parseSilentInt(getAttribute(router, "zoomToLoadTiles"), 16);
//		int desirable = parseSilentInt(getAttribute(router, "memoryLimitInMB"), 0);
//		if (desirable != 0) {
//			memoryLimitation = desirable * (1 << 20);
//		} else {
//			memoryLimitation = memoryLimitMB * (1 << 20);
//		}
//		planRoadDirection = parseSilentInt(getAttribute(router, "planRoadDirection"), 0);
//	}
//}*/
//
//
//RoutingConfiguration::~RoutingConfiguration() {
//}
//
//struct RoutingRule {
//	QString tagName;
//	QString t;
//	QString v;
//	QString param;
//	QString value1;
//	QString value2;
//	QString type;
//};
//
//bool checkTag(QString pname) {
//	return "select" == pname || "if" == pname ||
//			"ifnot"  == pname || "ge" == pname || "le" == pname;
//}
//
//void addSubclause(const RoutingRule& rr, std::shared_ptr<RouteAttributeContext> ctx) {
//	bool nt = "ifnot" == rr.tagName;
//	std::shared_ptr<RouteAttributeEvalRule> ptr = ctx->getLastRule();
//	if (rr.param != "") {
//		ptr->registerAndParamCondition(rr.param, nt);
//	}
//	if (rr.t != "") {
//		ptr->registerAndTagValueCondition(rr.t, rr.v, nt);
//	}
//	if (rr.tagName == "ge") {
//		ptr->registerGreatCondition(rr.value1, rr.value2, rr.type);
//	} else if (rr.tagName == "le") {
//		ptr->registerLessCondition(rr.value1, rr.value2, rr.type);
//	}
//}
//
//RoutingVehicleConfig::GeneralRouterProfile parseProfile(QString a){
//	if(a == "pedestrian") {
//		return RoutingVehicleConfig::GeneralRouterProfile::PEDESTRIAN;
//	} else if(a == "bicycle") {
//		return RoutingVehicleConfig::GeneralRouterProfile::BICYCLE;
//	}
//	return RoutingVehicleConfig::GeneralRouterProfile::CAR;
//}
//
//void parseRoutingRule(QXmlStreamReader& parser, std::shared_ptr<RoutingVehicleConfig> currentRouter, RouteDataObjectAttribute attr,
//		QStack<RoutingRule >& stack) {
//	QString pname = parser.name().toString();
//	if (checkTag(pname)) {
//		ASSERT(attr != RouteDataObjectAttribute::UKNOWN, "Select tag filter outside road attribute < " << \
//				pname.toStdString() << " > : "<< parser.lineNumber());
//		RoutingRule rr;
//		rr.tagName = pname;
//		rr.t = parser.attributes().value("t").toString();
//		rr.v = parser.attributes().value("v").toString();
//		rr.param = parser.attributes().value("param").toString();
//		rr.value1 = parser.attributes().value("value1").toString();
//		rr.value2 = parser.attributes().value("value2").toString();
//		rr.type = parser.attributes().value("type").toString();
//
//		std::shared_ptr<RouteAttributeContext> ctx = currentRouter->getObjContext(attr);
//		if("select" == rr.tagName) {
//			QString val = parser.attributes().
//					value( "value").toString();
//			QString type = parser.attributes().
//					value( "type").toString();
//			ctx->registerNewRule(val, type);
//			addSubclause(rr, ctx);
//			for (int i = 0; i < stack.size(); i++) {
//				addSubclause(stack.at(i), ctx);
//			}
//		} else if (stack.size() > 0 && stack.top().tagName == "select") {
//			addSubclause(rr, ctx);
//		}
//		stack.push(rr);
//	}
//}
//
//
//
//
//} /* namespace OsmAnd */
