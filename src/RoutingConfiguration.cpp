/*
 * RoutingConfiguration.cpp
 *
 *  Created on: Mar 4, 2013
 *      Author: victor
 */

#include "RoutingConfiguration.h"
#include <qxmlstream.h>
#include <qstack.h>
#include <qstringlist.h>
#include <RoutingVehicleConfig.h>


namespace OsmAnd {

QString RoutingConfiguration::getAttribute(RoutingVehicleConfig& router, QString& propertyName) {
	if (router->containsAttribute(propertyName)) {
		return router->getAttribute(propertyName);
	}
	return attributes[propertyName];
}

int RoutingConfiguration::parseSilentInt(QString t, int v) {
	if (t.length() == 0) {
		return v;
	}
	return t.toInt();
}

float RoutingConfiguration::parseSilentFloat(QString t, float v) {
	if (t.length() == 0) {
		return v;
	}
	return t.toFloat();
}


RoutingConfiguration::RoutingConfiguration(std::shared_ptr<RoutingConfigurationFile> file, QString router, double direction = -720,
			int memoryLimitMB = 30, QMap<QString, QString>& params){
		routerName = file->routers.contains(router) ? router : file->defaultRouterName;
		this->router = file->routers[routerName];
		if (!params.isEmpty()) {
			this->router = this->router.build(params);
		}
		attributes["routerName"] = router;
		attributes.putAll(file->attributes);
		recalculateDistance = parseSilentFloat(getAttribute(router, "recalculateDistanceHelp"),
				10000);
		heuristicCoefficient = parseSilentFloat(getAttribute(router, "heuristicCoefficient"),
				1);
		zoomToLoadTiles = parseSilentInt(getAttribute(router, "zoomToLoadTiles"), 16);
		int desirable = parseSilentInt(getAttribute(router, "memoryLimitInMB"), 0);
		if (desirable != 0) {
			memoryLimitation = desirable * (1 << 20);
		} else {
			memoryLimitation = memoryLimitMB * (1 << 20);
		}
		planRoadDirection = parseSilentInt(getAttribute(router, "planRoadDirection"), 0);
	}


RoutingConfiguration::~RoutingConfiguration() {
}

struct RoutingRule {
	QString tagName;
	QString t;
	QString v;
	QString param;
	QString value1;
	QString value2;
	QString type;
};

bool checkTag(QString pname) {
	return "select" == pname || "if" == pname ||
			"ifnot" == pname || "ge" == pname || "le" == pname;
}

void addSubclause(const RoutingRule& rr, shared_ptr<RouteAttributeContext> ctx) {
	bool nt = "ifnot" == rr.tagName;
	RouteAttributeEvalRule* ptr = ctx->getLastRule().get();
	if (rr.param != "") {
		ptr->registerAndParamCondition(rr.param, nt);
	}
	if (rr.t != "") {
		ptr->registerAndTagValueCondition(rr.t, rr.v, nt);
	}
	if (rr.tagName == "ge") {
		ptr->registerGreatCondition(rr.value1, rr.value2, rr.type);
	} else if (rr.tagName == "le") {
		ptr->registerLessCondition(rr.value1, rr.value2, rr.type);
	}
}

RoutingVehicleConfig::GeneralRouterProfile parseProfile(QString a){
	if(a == "pedestrian") {
		return RoutingVehicleConfig::GeneralRouterProfile::PEDESTRIAN;
	} else if(a == "bicycle") {
		return RoutingVehicleConfig::GeneralRouterProfile::BICYCLE;
	}
	return RoutingVehicleConfig::GeneralRouterProfile::CAR;
}


std::shared_ptr<RoutingVehicleConfig> parseRoutingProfile(QXmlStreamReader& parser,
		RoutingConfiguration::RoutingConfigurationFile& config) {
	QString currentSelectedRouter = parser.attributes().value("name").toString();
	QMap<QString, QString> attrs;

	for (auto it = parser.attributes().begin(); it != parser.attributes().end(); it++) {
		attrs[it->name().toString()] = it->value().toString();
	}
	config.defaultRouterName = parser.attributes().value("name").toString();
	RoutingVehicleConfig::GeneralRouterProfile c = parseProfile(parser.attributes().value("baseProfile").toString());
	std::shared_ptr<RoutingVehicleConfig> cconfig(new RoutingVehicleConfig(c, attrs));
	config.routers[currentSelectedRouter] =  cconfig;
	return cconfig;
}

void parseAttribute(QXmlStreamReader& parser,
		RoutingConfiguration::RoutingConfigurationFile& config, std::shared_ptr<RoutingVehicleConfig> currentRouter) {
	if(currentRouter == NULL) {
		currentRouter->addAttribute(parser.attributes().value("name").toString(),
				parser.attributes().value("value").toString());
	} else {
		config.attributes[parser.attributes().value("name").toString()] =
				parser.attributes().value("value").toString();
	}
}

void parseRoutingRule(QXmlStreamReader& parser, std::shared_ptr<RoutingVehicleConfig> currentRouter, RouteDataObjectAttribute attr,
		QStack<RoutingRule >& stack) {
	QString pname = parser.name().toString();
	if (checkTag(pname)) {
		ASSERT(attr != RouteDataObjectAttribute::UKNOWN, "Select tag filter outside road attribute < " << \
				pname.toStdString() << " > : "<< parser.lineNumber());
		RoutingRule rr;
		rr.tagName = pname;
		rr.t = parser.attributes().value("t").toString();
		rr.v = parser.attributes().value("v").toString();
		rr.param = parser.attributes().value("param").toString();
		rr.value1 = parser.attributes().value("value1").toString();
		rr.value2 = parser.attributes().value("value2").toString();
		rr.type = parser.attributes().value("type").toString();

		shared_ptr<RouteAttributeContext> ctx = currentRouter->getObjContext(attr);
		if("select" == rr.tagName) {
			QString val = parser.attributes().
					value( "value").toString();
			QString type = parser.attributes().
					value( "type").toString();
			ctx->registerNewRule(val, type);
			addSubclause(rr, ctx);
			for (int i = 0; i < stack.size(); i++) {
				addSubclause(stack.at(i), ctx);
			}
		} else if (stack.size() > 0 && stack.top().tagName == "select") {
			addSubclause(rr, ctx);
		}
		stack.push(rr);
	}
}

void parseRoutingParameter(QXmlStreamReader& parser, std::shared_ptr<RoutingVehicleConfig> currentRouter) {
	QString description = parser.attributes().value("description").toString();
	QString name = parser.attributes().value( "name").toString();
	QString id = parser.attributes().value( "id").toString();
	QString type = parser.attributes().value( "type").toString().toLower();
	if(type == "boolean") {
		currentRouter->registerBooleanParameter(id, name, description);
	} else if(type == "numeric") {
		QString values = parser.attributes().value( "values").toString();
		QString valueDescriptions = parser.attributes().value( "valueDescriptions").toString();
		QStringList strValues = values.split(",");
		QList<double> vls;
		for(QString& q : strValues) {
			vls.append(q.trimmed().toDouble());
		}
		currentRouter->registerNumericParameter(id, name, description, vls ,
				valueDescriptions.split(","));
	} else {
		ASSERT(0, "Unsupported routing parameter type - " << type.toStdString());
	}
}

RoutingConfiguration::RoutingConfigurationFile RoutingConfiguration::parseFromInputStream(QIODevice* is) {
	RoutingConfiguration::RoutingConfigurationFile config;
	QXmlStreamReader parser(is);
	std::shared_ptr<RoutingVehicleConfig> currentRouter;
	QStack<RoutingRule > rulesStack;
	RouteDataObjectAttribute currentAttribute;
	//	parser.setInput(is, "UTF-8");
	while (!parser.atEnd()) {
		parser.readNext();
		if (parser.isStartElement()) {
			QString name = parser.name().toString();
			if ("osmand_routing_config" == name) {
				config.defaultRouterName = parser.attributes().
						value("defaultProfile").toString();
			} else if ("routingProfile" == name) {
				currentRouter = parseRoutingProfile(parser, config);
			} else if ("attribute" == name) {
				parseAttribute(parser, config, currentRouter);
			} else if ("parameter" == name) {
				parseRoutingParameter(parser, currentRouter);
			} else if ("point" == name || "way" == name) {
				QString attribute = parser.attributes().
						value( "attribute").toString();
				currentAttribute = valueOfRouteDataObjectAttribute(attribute);
			} else {
				parseRoutingRule(parser, currentRouter, currentAttribute, rulesStack);
			}
		} else if (parser.isEndElement()) {
			QString pname = parser.name().toString();
			if (checkTag(pname)) {
				rulesStack.pop();
			}
		}
		parser.readNext();
	}
	return config;
}


} /* namespace OsmAnd */
