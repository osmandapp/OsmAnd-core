/*
 * RoutingVehicleConfig.h
 *
 *  Created on: Mar 4, 2013
 *      Author: victor
 */

#ifndef ROUTINGVEHICLECONFIG_H_
#define ROUTINGVEHICLECONFIG_H_
#include <OsmAndCore.h>
#include <qstring.h>
#include <qmap.h>
#include <qbitarray.h>
// TODO not use it! Replace with new implementation
#include "binaryRead.h"
#include "binaryRoutePlanner.h"
namespace OsmAnd {

class RouteAttributeEvalRule {
public:
	void registerAndParamCondition(QString s, bool nt);

	void registerAndTagValueCondition(QString tag, QString value, bool nt);

	void registerGreatCondition(QString v1, QString v2, QString type);

	void registerLessCondition(QString v1, QString v2, QString type);
};

enum RoutingParameterType {
		NUMERIC,
		BOOLEAN,
		SYMBOLIC
};

class RouteAttributeContext {
public:
	std::shared_ptr<RouteAttributeEvalRule> getLastRule();

	void registerNewRule(QString val, QString type);
};

struct RoutingParameter {
	QString id;
	QString name;
	QString description;
	RoutingParameterType type;
	QVector<QString> possibleValues;
	QVector<QString> possibleValueDescriptions;
};


enum RouteDataObjectAttribute {
	UKNOWN = 0, ROAD_SPEED, ROAD_PRIORITIES, ACCESS, OBSTACLES, ROUTING_OBSTACLES, ONEWAY
};

class OSMAND_CORE_API RoutingVehicleConfig {
	// field declaration
private:

	const QVector<RouteAttributeContext> objectAttributes;
	const QMap<QString, QString> attributes;
	const QMap<QString, RoutingParameter> parameters;
	const QMap<QString, int> universalRules;
	const QVector<QString> universalRulesById;
	// TOODO hash map
	const QMap<QString, QBitArray> tagRuleMask;
	const QVector<QObject> ruleToValue;
	// TODO hash map * 2

	// cached values
	bool restrictionsAware;
	float leftTurn;
	float roundaboutTurn;
	float rightTurn;
	float minDefaultSpeed;
	float maxDefaultSpeed;
public:
	bool containsAttribute(QString attribute);

	QString getAttribute(QString attribute);

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
	float getMinDefaultSpeed();

	/**
	 * Used for A* routing to predict h(x) : it should be great any g(x)
	 *
	 * @return maximum speed to calculate shortest distance
	 */
	float getMaxDefaultSpeed();

	/**
	 * aware of road restrictions
	 */
	bool areRestrictionsAware();

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
	RoutingVehicleConfig() :
			restrictionsAware(true), minDefaultSpeed(10), maxDefaultSpeed(10) {
	}

	RoutingVehicleConfig(GeneralRouterProfile profile, QMap<QString, QString>& attrs);

	void addAttribute(QString name, QString value);

	void registerBooleanParameter(QString& id, QString name, QString& description);

	void registerNumericParameter(QString& id, QString& name, QString& description, QList<double>& numbers,
			QList<QString> descriptions);

	shared_ptr<RouteAttributeContext> getObjContext(RouteDataObjectAttribute attr);

};


RouteDataObjectAttribute valueOfRouteDataObjectAttribute(QString& s) ;

} /* namespace OsmAnd */
#endif /* ROUTINGVEHICLECONFIG_H_ */
