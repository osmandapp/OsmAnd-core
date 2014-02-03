#ifndef _OSMAND_GENERAL_ROUTER_H
#define _OSMAND_GENERAL_ROUTER_H



#include "Common.h"
#include "common2.h"
#include <algorithm>
#include "boost/dynamic_bitset.hpp"
#include "Logging.h"
#include "binaryRead.h"
#include "binaryRoutePlanner.h"

struct RouteSegment;
class GeneralRouter;
class RouteAttributeContext;
typedef UNORDERED(map)<string, float> MAP_STR_FLOAT;
typedef UNORDERED(map)<string, string> MAP_STR_STR;
typedef UNORDERED(map)<int, int> MAP_INT_INT;
typedef UNORDERED(map)<string, int> MAP_STR_INT;
typedef boost::dynamic_bitset<> dynbitset;

#define DOUBLE_MISSING -1.1e9 // random big negative number


enum class RouteDataObjectAttribute : unsigned int {
	ROAD_SPEED = 0, //"speed"
	ROAD_PRIORITIES, // "priority"
	ACCESS, // "access"
	OBSTACLES, // "obstacle_time"
	ROUTING_OBSTACLES, // "obstacle"
	ONEWAY // "oneway"
};

enum class GeneralRouterProfile {
	CAR,
	PEDESTRIAN,
	BICYCLE
};

	
enum class RoutingParameterType {
	NUMERIC,
	BOOLEAN,
	SYMBOLIC
};

struct RoutingParameter {
	string id;
	string name;
	string description;
	RoutingParameterType type;
	vector<double> possibleValues; // Object TODO;
	vector<string> possibleValueDescriptions;
};

struct ParameterContext {
	MAP_STR_STR vars;
};

struct RouteAttributeExpression {
	static const int LESS_EXPRESSION;
	static const int GREAT_EXPRESSION;
		
	vector<string> values;
	int expressionType;
	string valueType;
	vector<double> cacheValues; 

	bool matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) ;

	double calculateExprValue(int id, dynbitset& types, ParameterContext& paramContext, GeneralRouter& router); 
};


class RouteAttributeEvalRule {
	friend class RouteAttributeContext;
private: 
	vector<string> parameters ;
	double selectValue ;
	string selectValueParam;
	string selectType;
	dynbitset filterTypes;
	dynbitset filterNotTypes;
		
	UNORDERED(set)<string> onlyTags;
	UNORDERED(set)<string> onlyNotTags;
	vector<RouteAttributeExpression> expressions;


	bool matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router);
	double eval(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router);
	double calcSelectValue(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router);

	bool checkAllTypesShouldBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) ;
	bool checkAllTypesShouldNotBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) ;
	bool checkNotFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) ;
	bool checkFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) ;
	bool checkExpressions(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) ;

};

class RouteAttributeContext {
	friend class GeneralRouter;


private:
	vector<RouteAttributeEvalRule> rules;
	ParameterContext paramContext ;
	GeneralRouter& router;

public: 
	RouteAttributeContext(GeneralRouter& r) : router(r) {
	}

private:
	double evaluate(dynbitset& types) {
		for (uint k = 0; k < rules.size(); k++) {
			RouteAttributeEvalRule r = rules[k];
			double o = rules[k].eval(types, paramContext, router);
			if (o != DOUBLE_MISSING) {
				return o;
			}
		}
		return DOUBLE_MISSING;
	}
	

	dynbitset convert(RoutingIndex* reg, std::vector<uint32_t>& types);

	double evaluate(SHARED_PTR<RouteDataObject> ro) {
		dynbitset local =  convert(ro->region, ro->types);
		return evaluate(local);
	}

	int evaluateInt(SHARED_PTR<RouteDataObject> ro, int defValue) {
		double d = evaluate(ro);
		if(d == DOUBLE_MISSING) {
			return defValue;
		}
		return (int)d;
	}


	double evaluateDouble(SHARED_PTR<RouteDataObject> ro, int defValue) {
		double d = evaluate(ro);
		if(d == DOUBLE_MISSING) {
			return defValue;
		}
		return d;
	}
};


class GeneralRouter {
	friend class RouteAttributeContext;
	friend class RouteAttributeEvalRule;
private:
	vector<RouteAttributeContext> objectAttributes;
	MAP_STR_STR attributes;
	UNORDERED(map)<string, RoutingParameter> parameters; 
	MAP_STR_INT universalRules;
	vector<tag_value> universalRulesById;
	UNORDERED(map)<string, dynbitset > tagRuleMask;
	vector<double> ruleToValue; // Object TODO;
	bool shortestRoute;
	
	UNORDERED(map)<RoutingIndex*, MAP_INT_INT> regionConvert;
	

	
public:
	// cached values
	bool _restrictionsAware ;
	double leftTurn;
	double roundaboutTurn;
	double rightTurn;
	double minDefaultSpeed ;
	double maxDefaultSpeed ;

	GeneralRouter() : _restrictionsAware(true), minDefaultSpeed(10), maxDefaultSpeed(10) {

	}

	bool containsAttribute(string attribute);
	
	string getAttribute(string attribute);
	
	/**
	 * return if the road is accepted for routing
	 */
	bool acceptLine(SHARED_PTR<RouteDataObject> way);
	
	/**
	 * return oneway +/- 1 if it is oneway and 0 if both ways
	 */
	int isOneWay(SHARED_PTR<RouteDataObject> road);
	
	/**
	 * return delay in seconds (0 no obstacles)
	 */
	float defineObstacle(SHARED_PTR<RouteDataObject> road, int point);
	
	/**
	 * return delay in seconds (0 no obstacles)
	 */
	float defineRoutingObstacle(SHARED_PTR<RouteDataObject> road, int point);

	/**
	 * return routing speed in m/s for vehicle for specified road
	 */
	float defineRoutingSpeed(SHARED_PTR<RouteDataObject> road);
	
	/**
	 * return real speed in m/s for vehicle for specified road
	 */
	float defineVehicleSpeed(SHARED_PTR<RouteDataObject> road);
	
	/**
	 * define priority to multiply the speed for g(x) A* 
	 */
	float defineSpeedPriority(SHARED_PTR<RouteDataObject> road);

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
	bool restrictionsAware();
	
	/**
	 * Calculate turn time 
	 */
	double calculateTurnTime(SHARED_PTR<RouteSegment> segment, int segmentEnd, 
		SHARED_PTR<RouteSegment> prev, int prevSegmentEnd);

private :
	double parseValue(string value, string type) ;

	double parseValueFromTag(uint id, string type, GeneralRouter& router);

	uint registerTagValueAttribute(const tag_value& r);

	RouteAttributeContext& getObjContext(RouteDataObjectAttribute a) {
		return objectAttributes[(unsigned int)a];
	}

};



#endif /*_OSMAND_GENERAL_ROUTER_H*/