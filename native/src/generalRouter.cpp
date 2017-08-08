#ifndef _OSMAND_GENERAL_ROUTER_CPP
#define _OSMAND_GENERAL_ROUTER_CPP
#include "generalRouter.h"
#include "binaryRoutePlanner.h"
#include "routeSegment.h"
#include <sstream>
#include <cmath>
#include "Logging.h"

const int RouteAttributeExpression::LESS_EXPRESSION = 1;
const int RouteAttributeExpression::GREAT_EXPRESSION = 2;
const int RouteAttributeExpression::EQUAL_EXPRESSION = 3;

const double GeneralRouterConstatns::CAR_SHORTEST_DEFAULT_SPEED = 55/3.6f;
const char* GeneralRouterConstatns::USE_SHORTEST_WAY = "short_way";
const char* GeneralRouterConstatns::USE_HEIGHT_OBSTACLES = "height_obstacles";
const char* GeneralRouterConstatns::ALLOW_PRIVATE = "allow_private";


GeneralRouter::GeneralRouter() : profile(GeneralRouterProfile::CAR), _restrictionsAware(true), heightObstacles(false), minDefaultSpeed(10),  maxDefaultSpeed(10), allowPrivate(false) {
}

GeneralRouter::GeneralRouter(const GeneralRouterProfile profile, const MAP_STR_STR& attributes) : profile(GeneralRouterProfile::CAR), _restrictionsAware(true), heightObstacles(false), minDefaultSpeed(10),  maxDefaultSpeed(10), allowPrivate(false) {
    
    this->profile = profile;
    MAP_STR_STR::const_iterator it = attributes.begin();
    for(;it != attributes.end(); it++) {
        addAttribute(it->first, it->second);
    }
    for (int i = 0; i < (int)RouteDataObjectAttribute::COUNT; i++) {
        newRouteAttributeContext();
    }
}

GeneralRouter::GeneralRouter(const GeneralRouter& parent, const MAP_STR_STR& params) : profile(GeneralRouterProfile::CAR), _restrictionsAware(true), heightObstacles(false), minDefaultSpeed(10),  maxDefaultSpeed(10), allowPrivate(false) {
    
    this->profile = parent.profile;
    MAP_STR_STR::const_iterator it = parent.attributes.begin();
    for(;it != parent.attributes.end(); it++) {
        addAttribute(it->first, it->second);
    }

    universalRules = parent.universalRules;
    universalRulesById = parent.universalRulesById;
    tagRuleMask = parent.tagRuleMask;
    ruleToValue = parent.ruleToValue;
    parameters = parent.parameters;
    
    for (int i = 0; i < (int)RouteDataObjectAttribute::COUNT; i++) {
        newRouteAttributeContext();
    }
        
    this->allowPrivate = parseBool(params, GeneralRouterConstatns::ALLOW_PRIVATE, false);
    this->shortestRoute = parseBool(params, GeneralRouterConstatns::USE_SHORTEST_WAY, false);
    this->heightObstacles = parseBool(params, GeneralRouterConstatns::USE_HEIGHT_OBSTACLES, false);
    if (shortestRoute) {
        maxDefaultSpeed = min(GeneralRouterConstatns::CAR_SHORTEST_DEFAULT_SPEED, this->maxDefaultSpeed);
    }
}

float parseFloat(MAP_STR_STR attributes, string key, float def) {
	if(attributes.find(key) != attributes.end() && attributes[key] != "") {
		return atof(attributes[key].c_str());
	}
	return def;
}

float parseFloat(string value, float def) {
    if (value != "") {
        return atof(value.c_str());
    }
    return def;
}

bool parseBool(MAP_STR_STR attributes, string key, bool def) {
	if (attributes.find(key) != attributes.end() && attributes[key] != "") {
		return attributes[key] == "true";
	}
	return def;
}

bool parseBool(string value, bool def) {
    if (value != "") {
        return value == "true";
    }
    return def;
}

string parseString(MAP_STR_STR attributes, string key, string def) {
	if (attributes.find(key) != attributes.end() && attributes[key] != "") {
		return attributes[key];
	}
	return def;
}

string parseString(string value, string def) {
    if (value != "") {
        return value;
    }
    return def;
}

dynbitset& increaseSize(dynbitset& t, uint targetSize) {
	if(t.size() < targetSize) {
		t.resize(targetSize);
	}
	return t;
}

dynbitset& align(dynbitset& t, uint targetSize) {
	if(t.size() < targetSize) {
		t.resize(targetSize);
	} else if(t.size() > targetSize) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Bitset %d is longer than target %d", t.size(), targetSize);
	}
	return t;
}


double parseValue(string value, string type) {
	double vl = -1;
	if("speed" == type) {
		vl = parseSpeed(value, vl);
	} else if("weight" == type) {
		vl = RouteDataObject::parseWeightInTon(value, vl);
	} else if("length" == type) {
		vl = RouteDataObject::parseLength(value, vl);
	} else {
		int i = findFirstNumberEndIndex(value);
		if (i > 0) {
			// could be negative
			return atof(value.substr(0, i).c_str());
		}
	}
	if(vl == -1) {
		return DOUBLE_MISSING;
	}
	return vl;
}


void GeneralRouter::addAttribute(string k, string v) {
	attributes[k] = v;
	if (k == "restrictionsAware") {
		_restrictionsAware = parseBool(attributes, k, _restrictionsAware);
	} else if (k == "leftTurn") {
		leftTurn = parseFloat(attributes, k, leftTurn);
	} else if (k == "rightTurn") {
		rightTurn = parseFloat(attributes, k, rightTurn);
	} else if (k == "roundaboutTurn") {
		roundaboutTurn = parseFloat(attributes, k, roundaboutTurn);
	} else if (k == "minDefaultSpeed") {
		minDefaultSpeed = parseFloat(attributes, k, minDefaultSpeed * 3.6f) / 3.6f;
	} else if (k =="maxDefaultSpeed") {
		maxDefaultSpeed = parseFloat(attributes, k, maxDefaultSpeed * 3.6f) / 3.6f;
	}
}

void toStr(std::ostringstream& s, UNORDERED(set)<string>& v) {
	s << "[";
	UNORDERED(set)<string>::iterator i = v.begin();
	for(; i != v.end(); i++) {
		if(i != v.begin()) s <<", ";
		s << (string)*i;
	}

	s << "]";
}

void RouteAttributeEvalRule::printRule(GeneralRouter* r) {
	std::ostringstream s;
	s << " Select ";
	if(selectValue == DOUBLE_MISSING) {
		s << selectValueDef;
	} else {
		s << selectValue;
	}

	bool f = true;
	for(uint k = 0; k < filterTypes.size(); k++) {
		if(filterTypes.test(k)) {
			if(f) {
				s << " if ";
				f = !f;
			}
			tag_value key = r->universalRulesById[k];
			s << key.first << "/" << key.second;
		}
	}
	f = true;
	for(uint k = 0; k < filterNotTypes.size(); k++) {		
		if(filterNotTypes.test(k)) {
			if(f) {
				s << " if ";
				f = !f;
			}
			tag_value key = r->universalRulesById[k];
			s << key.first << "/" << key.second;
		}
	}
	for(uint k = 0; k < parameters.size(); k++) {
		s << " param=" << parameters[k];
	}
	if(onlyTags.size() > 0) {
		s << " match tag = ";
		toStr(s, onlyTags);
	}
	if(onlyNotTags.size() > 0) {
		s << " not match tag = ";
		toStr(s, onlyNotTags);
	}
	if(expressions.size() > 0) {
		s << " subexpressions " << expressions.size();
	}
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "%s", s.str().c_str());
}

RouteAttributeExpression::RouteAttributeExpression(vector<string>&vls, int type, string vType) : 
		 values(vls), expressionType(type), valueType(vType){
	cacheValues.resize(vls.size());
	for (uint i = 0; i < vls.size(); i++) {
		if(vls[i][0] != '$' && vls[i][0] != ':') {
			double o = parseValue(vls[i], valueType);
			cacheValues[i] = o;
		} else {
			cacheValues[i] = DOUBLE_MISSING;
		}
	}
}

void RouteAttributeEvalRule::registerAndTagValueCondition(GeneralRouter* r, string tag, string value, bool nt) {
	tagValueCondDefTag.push_back(tag);
	tagValueCondDefValue.push_back(value);
	tagValueCondDefNot.push_back(nt);
	if (value.empty()) {
		if (nt) {
			onlyNotTags.insert(tag);
		} else {
			onlyTags.insert(tag);
		}
	} else {
		tag_value t = tag_value(tag, value);
		int vtype = r->registerTagValueAttribute(t);
		if(nt) {
			increaseSize(filterNotTypes, vtype + 1).set(vtype);
		} else {
			increaseSize(filterTypes, vtype + 1).set(vtype);
		}
	}
}

void RouteAttributeEvalRule::registerParamConditions(vector<string>& params) {
	parameters.insert(parameters.end(), params.begin(), params.end());
}

void RouteAttributeEvalRule::registerAndParamCondition(string param, bool nt) {
    param = nt ? "-" + param : param;
    parameters.push_back(param);
}

void RouteAttributeEvalRule::registerSelectValue(string value, string type) {
	this->selectType = type;
	this->selectValueDef = value;
	if (selectValueDef.length() > 0 && (selectValueDef[0] == '$' || selectValueDef[0] == ':')) {
		// init later
		selectValue = DOUBLE_MISSING;
	} else {
		selectValue = parseValue(value, type);
		if(selectValue == DOUBLE_MISSING) {
		//	System.err.println("Routing.xml select value '" + value+"' was not registered");
		}
	}
}


double GeneralRouter::parseValueFromTag(uint id, string type, GeneralRouter* router) {
	while (ruleToValue.size() <= id) {
		ruleToValue.push_back(DOUBLE_MISSING);
	}
	double res = ruleToValue[id];
	if (res == DOUBLE_MISSING) {
		tag_value v = router->universalRulesById[id];
		res = parseValue(v.second, type);
		if (res == DOUBLE_MISSING) {
			res = DOUBLE_MISSING - 1;
		}
		ruleToValue[id] = res;
	}
	if (res == DOUBLE_MISSING - 1) {
		return DOUBLE_MISSING;
	}
	return res;
}


bool GeneralRouter::containsAttribute(string attribute) {
	return attributes.find(attribute) != attributes.end();
}

string GeneralRouter::getAttribute(string attribute) {
	return attributes[attribute];
}

bool GeneralRouter::acceptLine(SHARED_PTR<RouteDataObject> way) {
	int res = getObjContext(RouteDataObjectAttribute::ACCESS).evaluateInt(way, 0);
	if(impassableRoadIds.find(way->id) != impassableRoadIds.end()) {
		return false;
	}
	return res >= 0;
}

int GeneralRouter::isOneWay(SHARED_PTR<RouteDataObject> road) {
	return getObjContext(RouteDataObjectAttribute::ONEWAY).evaluateInt(road, 0);
}

double GeneralRouter::defineObstacle(SHARED_PTR<RouteDataObject> road, uint point) {
	if(road->pointTypes.size() > point && road->pointTypes[point].size() > 0){
		return getObjContext(RouteDataObjectAttribute::OBSTACLES).evaluateDouble(road->region, road->pointTypes[point], 0);
	}
	return 0;
}

double GeneralRouter::defineHeightObstacle(SHARED_PTR<RouteDataObject> road, uint startIndex, uint endIndex) {
	if(!heightObstacles) {
 		return 0;
 	}
 	vector<double> heightArray = road->calculateHeightArray();
 	if(heightArray.size() == 0 ) {
 		return 0;
 	}
 	
 	double sum = 0;
 	int knext;
 	vector<uint32_t> types;
 	RouteAttributeContext& objContext = getObjContext(RouteDataObjectAttribute::OBSTACLE_SRTM_ALT_SPEED);
 	for(uint k = startIndex; k != endIndex; k = knext) {
 		knext = startIndex < endIndex ? k + 1 : k - 1;
 		double dist = startIndex < endIndex ? heightArray[2 * knext] : heightArray[2 * k]  ;
 		double diff = heightArray[2 * knext + 1] - heightArray[2 * k + 1] ;
 		if(diff != 0 && dist > 0) {
 			double incl = abs(diff / dist);
 			int percentIncl = (int) (incl * 100);
 			percentIncl = (percentIncl + 2)/ 3 * 3 - 2; // 1, 4, 7, 10, .   
 			if(percentIncl >= 1) {
 				objContext.paramContext.incline = diff > 0 ? percentIncl : -percentIncl;
 				sum += objContext.evaluateDouble(road, 0) * (diff > 0 ? diff : -diff);
 			}
 		}
 	}
 	return sum;
 }




double GeneralRouter::defineRoutingObstacle(SHARED_PTR<RouteDataObject> road, uint point) {
	if(road->pointTypes.size() > point && road->pointTypes[point].size() > 0){
		return getObjContext(RouteDataObjectAttribute::ROUTING_OBSTACLES).evaluateDouble(road->region, road->pointTypes[point], 0);
	}
	return 0;
}

double GeneralRouter::defineRoutingSpeed(SHARED_PTR<RouteDataObject> road) {
	return min(defineVehicleSpeed(road), maxDefaultSpeed);
}

double GeneralRouter::defineVehicleSpeed(SHARED_PTR<RouteDataObject> road) {
	return getObjContext(RouteDataObjectAttribute::ROAD_SPEED) .evaluateDouble(road, getMinDefaultSpeed());
}

double GeneralRouter::definePenaltyTransition(SHARED_PTR<RouteDataObject> road) {
	if(!isObjContextAvailable(RouteDataObjectAttribute::PENALTY_TRANSITION)) {
		return 0;
	}
	return getObjContext(RouteDataObjectAttribute::PENALTY_TRANSITION) .evaluateDouble(road, 0);
}


double GeneralRouter::defineSpeedPriority(SHARED_PTR<RouteDataObject> road) {
	return getObjContext(RouteDataObjectAttribute::ROAD_PRIORITIES).evaluateDouble(road, 1.);
}

double GeneralRouter::getMinDefaultSpeed() {
	return minDefaultSpeed;
}

double GeneralRouter::getMaxDefaultSpeed() {
	return maxDefaultSpeed;
}

bool GeneralRouter::restrictionsAware() {
	return _restrictionsAware;
}


double GeneralRouter::calculateTurnTime(SHARED_PTR<RouteSegment> segment, int segmentEnd, 
		SHARED_PTR<RouteSegment> prev, int prevSegmentEnd) {
	double ts = definePenaltyTransition(segment->getRoad());
	double prevTs = definePenaltyTransition(prev->getRoad());
	if(prevTs != ts) {
		return abs(ts - prevTs) / 2;
	}
	// if(prev->road->pointTypes.size() > (uint)prevSegmentEnd && prev->road->pointTypes[prevSegmentEnd].size() > 0){
	// 	RoutingIndex* reg = prev->getRoad()->region;
	// 	vector<uint32_t> pt = prev->road->pointTypes[prevSegmentEnd];
	// 	for (uint i = 0; i < pt.size(); i++) {
	// 		tag_value r = reg->decodingRules[pt[i]];
	// 		if ("highway" == r.first && "traffic_signals" == r.second) {
	// 			// traffic signals don't add turn info
	// 			return 0;
	// 		}
	// 	}
	// }
	
	
	if(segment->getRoad()->roundabout() && !prev->getRoad()->roundabout()) {
		double rt = roundaboutTurn;
		if(rt > 0) {
			return rt;
		}
	}
	if (leftTurn > 0 || rightTurn > 0) {
		double a1 = segment->getRoad()->directionRoute(segment->getSegmentStart(), segment->getSegmentStart() < segmentEnd);
		double a2 = prev->getRoad()->directionRoute(prevSegmentEnd, prevSegmentEnd < prev->getSegmentStart());
		double diff = abs(alignAngleDifference(a1 - a2 - M_PI));
		// more like UT
		if (diff > 2 * M_PI / 3) {
			return leftTurn;
		} else if (diff > M_PI / 2) {
			return rightTurn;
		}
		return 0;
	}
	return 0;
}

void GeneralRouter::printRules() {
    for (uint k = 0; k < objectAttributes.size(); k++) {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "RouteAttributeContext  %d", k + 1);
        objectAttributes[k]->printRules();
    }
}

uint GeneralRouter::registerTagValueAttribute(const tag_value& r) {
	string key = r.first + "$" + r.second;
	MAP_STR_INT::iterator it = universalRules.find(key);
	if (it != universalRules.end()) {
		return ((uint)it->second);
	}
	int id = (int)universalRules.size();
	universalRulesById.push_back(r);
	universalRules[key] = id;
	dynbitset& d = increaseSize(tagRuleMask[r.first], id + 1);
	d.set(id);
	return id;
}

dynbitset RouteAttributeContext::convert(RoutingIndex* reg, std::vector<uint32_t>& types) {
	dynbitset b(router->universalRules.size());
    MAP_INT_INT map;
    if (router->regionConvert.find(reg) != router->regionConvert.end()) {
        map = router->regionConvert[reg];
    } else {
        router->regionConvert[reg] = map;
    }
	for (uint k = 0; k < types.size(); k++) {
		MAP_INT_INT::iterator nid = map.find(types[k]);
		int vl;
		if(nid == map.end()){
			auto& r = reg->decodingRules[types[k]];
			vl = router->registerTagValueAttribute(tag_value(r.getTag(), r.getValue()));
			map[types[k]] = vl;
		} else {
			vl = nid->second;
		}
		increaseSize(b, (uint)router->universalRules.size()).set(vl);
	}
	return b;
}

double RouteAttributeEvalRule::eval(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	if (matches(types, paramContext, router)) {
		return calcSelectValue(types, paramContext, router);
	}
	return DOUBLE_MISSING;
}


double RouteAttributeEvalRule::calcSelectValue(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	if(selectValue != DOUBLE_MISSING) {
		return selectValue;
	}
	if (selectValueDef.length() > 0 && selectValueDef[0]=='$') {
		UNORDERED(map)<string, dynbitset >::iterator ms = router->tagRuleMask.find(selectValueDef.substr(1));
		if (ms != router->tagRuleMask.end() && align(ms->second, types.size()).intersects(types)) {
			dynbitset findBit(ms->second.size());
			findBit |= ms->second;
			findBit &= types;
			uint value = findBit.find_first();
			double vd = router->parseValueFromTag(value, selectType, router);;
			return vd;
		}
	} else if (selectValueDef.length() > 0 && selectValueDef[0]==':') {
		string p = selectValueDef.substr(1);
		MAP_STR_STR::iterator it = paramContext.vars.find(p);
		if (it != paramContext.vars.end()) {
			selectValue = parseValue(it->second, selectType);
		} else {
			return DOUBLE_MISSING;
		}
	}
	return selectValue;
}

bool RouteAttributeExpression::matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	double f1 = calculateExprValue(0, types, paramContext, router);
	double f2 = calculateExprValue(1, types, paramContext, router);
	if(f1 == DOUBLE_MISSING || f2 == DOUBLE_MISSING) {
		return false;
	}

	if (expressionType == LESS_EXPRESSION) {
		return f1 <= f2;
	} else if (expressionType == GREAT_EXPRESSION) {
		return f1 >= f2;
	} else if (expressionType == EQUAL_EXPRESSION) {
		return f1 == f2;
	}
	return false;
}


double RouteAttributeExpression::calculateExprValue(int id, dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	
	double cacheValue = cacheValues[id];
	string value = values[id];
	double o = DOUBLE_MISSING;
	if(cacheValue != DOUBLE_MISSING) {
		return cacheValue;
	}
	if (value.length() > 0 && value[0]=='$') {
		UNORDERED(map)<string, dynbitset >::iterator ms = router->tagRuleMask.find(value.substr(1));
		if (ms != router->tagRuleMask.end() && align(ms->second, types.size()).intersects(types)) {
			dynbitset findBit(ms->second.size());
			findBit |= ms->second;
			findBit &= types;
			uint value = findBit.find_first();
			return router->parseValueFromTag(value, valueType, router);
		}
	} else if(value == ":incline") {
		return paramContext.incline;
	} else if (value.length() > 0 && value[0]==':') {
		string p = value.substr(1);
		MAP_STR_STR::iterator it = paramContext.vars.find(p);
		if (it != paramContext.vars.end()) {
			o = parseValue(it->second, valueType);
		} else {
			return DOUBLE_MISSING;		
		}
	}
	cacheValues[id] = o;
	return o;
} 

bool RouteAttributeEvalRule::matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	if(!checkAllTypesShouldBePresent(types, paramContext, router)) {
		return false;
	}
	if(!checkAllTypesShouldNotBePresent(types, paramContext, router)) {
		return false;
	}
	if(!checkFreeTags(types, paramContext, router)) {
		return false;
	}
	if(!checkNotFreeTags(types, paramContext, router)) {
		return false;
	}
	if(!checkExpressions(types, paramContext, router)) {
		return false;
	}
	return true;
}

bool RouteAttributeEvalRule::checkExpressions(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	for(uint i = 0; i < expressions.size(); i++) {
		if(!expressions[i].matches(types, paramContext, router)) {
			return false;
		}
	}
	return true;
}

bool RouteAttributeEvalRule::checkFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	for(UNORDERED(set)<string>::iterator it = onlyTags.begin(); it != onlyTags.end(); it++) {
		UNORDERED(map)<string, dynbitset >::iterator ms = router->tagRuleMask.find(*it);
		if (ms == router->tagRuleMask.end() || !align(ms->second, types.size()).intersects(types)) {
			return false;
		}
	}
	return true;
}

bool RouteAttributeEvalRule::checkNotFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	for(UNORDERED(set)<string>::iterator it = onlyNotTags.begin(); it != onlyNotTags.end(); it++) {
		UNORDERED(map)<string, dynbitset >::iterator ms = router->tagRuleMask.find(*it);
		if (ms != router->tagRuleMask.end() && align(ms->second, types.size()).intersects(types)) {
			return false;
		}
	}
	return true;
}

bool RouteAttributeEvalRule::checkAllTypesShouldNotBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	if(align(filterNotTypes, types.size()).intersects(types)) {
		return false;
	}
	return true;
}

bool RouteAttributeEvalRule::checkAllTypesShouldBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter* router) {
	// Bitset method subset is missing "filterTypes.isSubset(types)"    
	// reset previous evaluation
	return align(filterTypes, types.size()).is_subset_of(types);
	// evalFilterTypes.clear();
	// evalFilterTypes |= filterTypes;
	//// evaluate bit intersection and check if filterTypes contained as set in types
	// evalFilterTypes &= types;
	// if(!evalFilterTypes == filterTypes) {
	//	return false;
	//}
	// return true;
}

#endif /*_OSMAND_GENERAL_ROUTER_CPP*/
