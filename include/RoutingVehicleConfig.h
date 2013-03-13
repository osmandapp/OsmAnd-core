/*
 * RoutingVehicleConfig.h
 *
 *  Created on: Mar 4, 2013
 *      Author: victor
 */

#ifndef ROUTINGVEHICLECONFIG_H_
#define ROUTINGVEHICLECONFIG_H_
#include <memory>
#include <qstring.h>
#include <qmap.h>
#include <qlist.h>
#include <qbitarray.h>
#include <qvector.h>

#include <OsmAndCore.h>
#include <ObfRoutingSection.h>

// TODO not use it! Replace with new implementation
// #include "binaryRead.h"
// #include "binaryRoutePlanner.h"
namespace OsmAnd {



enum RoutingParameterType {
	NUMERIC, BOOLEAN, SYMBOLIC
};

class RouteAttributeEvalRule;


struct RoutingParameter {
	QString id;
	QString name;
	QString description;
	RoutingParameterType type;
	QVector<QString> possibleValues;
	QVector<QString> possibleValueDescriptions;
};

struct RouteSelectValue {
	double doubleValue;
	bool isDoubleValue;
	QString selectValue;
	RouteSelectValue(QString v) {
		selectValue = v;
		doubleValue = v.toDouble(&isDoubleValue);
	}

};

enum RouteDataObjectAttribute {
	UKNOWN = 0, ROAD_SPEED = 1, ROAD_PRIORITIES, ACCESS, OBSTACLES, ROUTING_OBSTACLES, ONEWAY, LAST
};

class OSMAND_CORE_API RoutingVehicleConfig {
	// field declaration
private:

	QVector<RouteAttributeContext> objectAttributes;
	QMap<QString, QString> attributes;
	QMap<QString, RoutingParameter> parameters;
	// TODO hash map
	QMap<QString, int> universalRules;
	QVector<QString> universalRulesById;
	// TODO hash map
	QMap<QString, QBitArray> tagRuleMask;
	QVector<RouteSelectValue> ruleToValue;


	// cached values
	bool restrictionsAware;
	float leftTurn;
	float roundaboutTurn;
	float rightTurn;
	float minDefaultSpeed;
	float maxDefaultSpeed;
public:
	bool containsAttribute(QString& attribute);

	QString& getAttribute(QString& attribute);

	/**
	 * return if the road is accepted for routing
	 */
	bool acceptLine(RouteDataObject& way);

	/**
	 * return oneway +/- 1 if it is oneway and 0 if both ways
	 */
	int isOneWay(RouteDataObject& road);

	/**
	 * return delay in seconds (0 no obstacles)
	 */
	float defineObstacle(RouteDataObject& road, int point);

	/**
	 * return delay in seconds (0 no obstacles)
	 */
	float defineRoutingObstacle(RouteDataObject& road, int point);

	/**
	 * return speed in m/s for vehicle for specified road
	 */
	float defineSpeed(RouteDataObject& road);

	/**
	 * define priority to multiply the speed for g(x) A*
	 */
	float defineSpeedPriority(RouteDataObject& road);

	/**
	 * Used for A* routing to calculate g(x)
	 *
	 * @return minimal speed at road in m/s
	 */
	float getMinDefaultSpeed() { return minDefaultSpeed; }

	/**
	 * Used for A* routing to predict h(x) : it should be great any g(x)
	 *
	 * @return maximum speed to calculate shortest distance
	 */
	float getMaxDefaultSpeed() { return maxDefaultSpeed; }

	/**
	 * aware of road restrictions
	 */
	bool areRestrictionsAware() { return restrictionsAware; }

	/**
	 * Calculate turn time
	 */
	double calculateTurnTime(RouteSegment& segment, int segmentEnd, RouteSegment& prev, int prevSegmentEnd);

	std::shared_ptr<RoutingVehicleConfig> build(QMap<QString, QString>& params);

	enum GeneralRouterProfile {
		CAR, PEDESTRIAN, BICYCLE
	};

	enum RoutingParameterType {
		NUMERIC, BOOLEAN, SYMBOLIC
	};

	virtual ~RoutingVehicleConfig();

	// Modify functionality used only to parse configuration
	RoutingVehicleConfig();

	RoutingVehicleConfig(RoutingVehicleConfig& parent, QMap<QString, QString>& params);

	void addAttribute(const QString& name,const  QString& value);

	void registerBooleanParameter(QString& id, QString& name, QString& description);

	void registerNumericParameter(QString& id, QString& name, QString& description, QList<double>& numbers,
			QList<QString> descriptions);

	std::shared_ptr<RouteAttributeContext> getObjContext(RouteDataObjectAttribute attr);

private:
	int registerTagValueAttribute(QString& tag, QString& value);
};

RouteDataObjectAttribute valueOfRouteDataObjectAttribute(QString& s);
} /* namespace OsmAnd */
#endif /* ROUTINGVEHICLECONFIG_H_ */
