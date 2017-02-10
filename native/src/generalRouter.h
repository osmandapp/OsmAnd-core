#ifndef _OSMAND_GENERAL_ROUTER_H
#define _OSMAND_GENERAL_ROUTER_H



#include "Common.h"
#include "common2.h"
#include <algorithm>
#include "boost/dynamic_bitset.hpp"
#include "Logging.h"
#include "binaryRead.h"

struct RouteSegment;
class GeneralRouter;
class RouteAttributeContext;

//#include "binaryRoutePlanner.h"

typedef UNORDERED(map)<string, float> MAP_STR_FLOAT;
typedef UNORDERED(map)<string, string> MAP_STR_STR;
typedef UNORDERED(map)<int, int> MAP_INT_INT;
typedef UNORDERED(map)<string, int> MAP_STR_INT;
typedef boost::dynamic_bitset<> dynbitset;

#define DOUBLE_MISSING -1.1e9 // random big negative number


enum class RouteDataObjectAttribute : unsigned int {
	ROAD_SPEED = 0, //"speed"
	ROAD_PRIORITIES = 1, // "priority"
	ACCESS = 2, // "access"
	OBSTACLES = 3, // "obstacle_time"
	ROUTING_OBSTACLES = 4, // "obstacle"
	ONEWAY = 5,// "oneway"
	PENALTY_TRANSITION = 6, // "penalty_transition"
	OBSTACLE_SRTM_ALT_SPEED = 7 // "obstacle_srtm_alt_speed"
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
	static const int EQUAL_EXPRESSION ;
		
	vector<string> values;
	int expressionType;
	string valueType;
	vector<double> cacheValues; 

	RouteAttributeExpression(vector<string>&vls, int type, string vType);

	bool matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) ;

	double calculateExprValue(int id, dynbitset& types, ParameterContext& paramContext, GeneralRouter* router); 
};


class RouteAttributeEvalRule {
	friend class RouteAttributeContext;

private: 
	vector<string> parameters ;
	double selectValue ;
	string selectValueDef ;
	string selectType;
	dynbitset filterTypes;
	dynbitset filterNotTypes;
		
	UNORDERED(set)<string> onlyTags;
	UNORDERED(set)<string> onlyNotTags;
	vector<RouteAttributeExpression> expressions;

	vector<string> tagValueCondDefValue;
	vector<string> tagValueCondDefTag;
	vector<bool> tagValueCondDefNot;


	bool matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router);
	double eval(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router);
	double calcSelectValue(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router);

	bool checkAllTypesShouldBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) ;
	bool checkAllTypesShouldNotBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) ;
	bool checkNotFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) ;
	bool checkFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) ;
	bool checkExpressions(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) ;

	void printRule(GeneralRouter* r);
public:
	void registerAndTagValueCondition(GeneralRouter* r, string tag, string value, bool nt); 

	// formated as [param1,-param2]
	void registerParamConditions(vector<string>& params); 

	void registerSelectValue(string selectValue, string selectType); 

	void registerExpression(RouteAttributeExpression& expression) {
		expressions.push_back(expression);
	}

	// registerGreatCondition, registerLessCondition

};

class RouteAttributeContext {
	friend class GeneralRouter;

private:
	vector<RouteAttributeEvalRule*> rules;
	ParameterContext paramContext ;
	GeneralRouter* router;

public: 
	RouteAttributeContext(GeneralRouter* r) : router(r) {
	}

	~RouteAttributeContext() {
		for (uint k = 0; k < rules.size(); k++) {
			delete rules[k];
		}
	}

	void registerParams(vector<string>& keys, vector<string>& vls) {
		for(uint i = 0; i < keys.size(); i++) {
			paramContext.vars[keys[i]] = vls[i];
		}
	}

	RouteAttributeEvalRule* newEvaluationRule() {
		RouteAttributeEvalRule* c = new RouteAttributeEvalRule();
		rules.push_back(c);
		return rules[rules.size() - 1];
	}

	void printRules() {
		for (uint k = 0; k < rules.size(); k++) {
			RouteAttributeEvalRule* r = rules[k];
			r->printRule(router);
		}
	}

private:
	double evaluate(dynbitset& types) {
		for (uint k = 0; k < rules.size(); k++) {			
			double o = rules[k]->eval(types, paramContext, router);			
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

	double evaluateDouble(RoutingIndex* reg, std::vector<uint32_t>& types, double defValue) {
		dynbitset local =  convert(reg, types);		
		double d = evaluate(local);
		if(d == DOUBLE_MISSING) {
			return defValue;
		}
		return d;
	}

	double evaluateDouble(SHARED_PTR<RouteDataObject> ro, double defValue) {
		double d = evaluate(ro);
		if(d == DOUBLE_MISSING) {
			return defValue;
		}
		return d;
	}
};

float parseFloat(MAP_STR_STR attributes, string key, float def);

bool parseBool(MAP_STR_STR attributes, string key, bool def);

string parseString(MAP_STR_STR attributes, string key, string def);


class GeneralRouter {
	friend class RouteAttributeContext;
	friend class RouteAttributeEvalRule;
	friend struct RouteAttributeExpression;
private:
	vector<RouteAttributeContext*> objectAttributes;
	MAP_STR_STR attributes;
	UNORDERED(map)<string, RoutingParameter> parameters; 
	MAP_STR_INT universalRules;
	vector<tag_value> universalRulesById;
	UNORDERED(map)<string, dynbitset > tagRuleMask;
	vector<double> ruleToValue; // Object TODO;
	
	UNORDERED(map)<RoutingIndex*, MAP_INT_INT> regionConvert;
		
public:
	// cached values
	bool _restrictionsAware ;
	double leftTurn;
	double roundaboutTurn;
	double rightTurn;
	double minDefaultSpeed ;
	double maxDefaultSpeed ;
	UNORDERED(set)<int64_t> impassableRoadIds;

	GeneralRouter() : _restrictionsAware(true), minDefaultSpeed(10), maxDefaultSpeed(10) {
	}

	~GeneralRouter() {
		for (uint k = 0; k < objectAttributes.size(); k++) {
			delete objectAttributes[k];
		}
	}

	RouteAttributeContext* newRouteAttributeContext() {
		RouteAttributeContext *c = new RouteAttributeContext(this);
		objectAttributes.push_back(c);
		return objectAttributes[objectAttributes.size() - 1];
	}

	void addAttribute(string k, string v) ;

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
	double defineObstacle(SHARED_PTR<RouteDataObject> road, uint point);
	
	/**
	 * return delay in seconds (0 no obstacles)
	 */
	double defineRoutingObstacle(SHARED_PTR<RouteDataObject> road, uint point);

	/**
	 * return routing speed in m/s for vehicle for specified road
	 */
	double defineRoutingSpeed(SHARED_PTR<RouteDataObject> road);

	/*
	* return transition penalty between different road classes in seconds
	*/
	double definePenaltyTransition(SHARED_PTR<RouteDataObject> road);
	
	/**
	 * return real speed in m/s for vehicle for specified road
	 */
	double defineVehicleSpeed(SHARED_PTR<RouteDataObject> road);
	
	/**
	 * define priority to multiply the speed for g(x) A* 
	 */
	double defineSpeedPriority(SHARED_PTR<RouteDataObject> road);

	/**
	 * Used for A* routing to calculate g(x)
	 * 
	 * @return minimal speed at road in m/s
	 */
	double getMinDefaultSpeed();



	/**
	 * Used for A* routing to predict h(x) : it should be great any g(x)
	 * 
	 * @return maximum speed to calculate shortest distance
	 */
	double getMaxDefaultSpeed();
	
	/**
	 * aware of road restrictions
	 */
	bool restrictionsAware();
	
	/**
	 * Calculate turn time 
	 */
	double calculateTurnTime(SHARED_PTR<RouteSegment> segment, int segmentEnd, 
		SHARED_PTR<RouteSegment> prev, int prevSegmentEnd);


	void printRules() {
		for (uint k = 0; k < objectAttributes.size(); k++) {
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "RouteAttributeContext  %d", k + 1);
			objectAttributes[k]->printRules();
		}
	}
private :

	double parseValueFromTag(uint id, string type, GeneralRouter* router);

	uint registerTagValueAttribute(const tag_value& r);

	bool isObjContextAvailable(RouteDataObjectAttribute a) {
		return objectAttributes.size() > (unsigned int)a;
	}
	RouteAttributeContext& getObjContext(RouteDataObjectAttribute a) {
		return *objectAttributes[(unsigned int)a];
	}

};


#endif /*_OSMAND_GENERAL_ROUTER_H*/