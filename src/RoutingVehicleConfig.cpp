/*
 * RoutingVehicleConfig.cpp
 *
 *  Created on: Mar 4, 2013
 *      Author: victor
 */

#include "RoutingVehicleConfig.h"


namespace OsmAnd {

namespace {
bool parseSilentBoolean(QString t, bool v) {
	if (t == "") {
		return v;
	}
	return t == "true";
}

float parseSilentFloat(QString t, float v) {
	if (t == "") {
		return v;
	}
	bool o;
	double d = t.toDouble(&o);
	if (!o) {
		return v;
	}
	return d;
}

static const int LESS_EXPRESSION = 1;
static const int GREAT_EXPRESSION = 2;

class RouteAttributeExpression {

public RouteAttributeExpression(String[] vs, String valueType, int expressionId) {
		this.expressionType = expressionId;
		this.values = vs;
		if (vs.length < 2) {
			throw new IllegalStateException("Expression should have at least 2 arguments");
		}
		this.cacheValues = new Number[vs.length];
		this.valueType = valueType;
		for (int i = 0; i < vs.length; i++) {
			if(!vs[i].startsWith("$") && !vs[i].startsWith(":")) {
				Object o = parseValue(vs[i], valueType);
				if (o instanceof Number) {
					cacheValues[i] = (Number) o;
				}
			}
		}
	}

private String[] values;private int expressionType;private String valueType;private Number[] cacheValues;

public boolean matches(BitSet types, ParameterContext paramContext) {
		double f1 = calculateExprValue(0, types, paramContext);
		double f2 = calculateExprValue(1, types, paramContext);
		if(Double.isNaN(f1) ||Double.isNaN(f2)) {
			return false;
		}
		if (expressionType == LESS_EXPRESSION) {
			return f1 <= f2;
		} else if (expressionType == GREAT_EXPRESSION) {
			return f1 >= f2;
		}
		return false;
	}

private double calculateExprValue(int id, BitSet types, ParameterContext paramContext) {
		String value = values[id];
		Number cacheValue = cacheValues[id];
		if(cacheValue != null) {
			return cacheValue.doubleValue();
		}
		Object o = null;
		if (value instanceof String && value.toString().startsWith("$")) {
			BitSet mask = tagRuleMask.get(value.toString().substring(1));
			if (mask != null && mask.intersects(types)) {
				BitSet findBit = new BitSet(mask.size());
				findBit.or(mask);
				findBit.and(types);
				int v = findBit.nextSetBit(0);
				o = parseValueFromTag(v, valueType);
			}
		} else if (value instanceof String && value.toString().startsWith(":")) {
			String p = ((String) value).substring(1);
			if (paramContext != null && paramContext.vars.containsKey(p)) {
				o = parseValue(paramContext.vars.get(p), value);
			}
		}

		if(o instanceof Number) {
			return ((Number)o).doubleValue();
		}
		return Double.NaN;
	}

};

class RouteAttributeEvalRule {
	QList<QString> parameters
	Object selectValue;
	QString selectType;
	QBitSet filterTypes;
	QBitSet filterNotTypes;
	QBitSet evalFilterTypes;
	QSet<String> onlyTags;
	QSet<String> onlyNotTags;
	QList<RouteAttributeExpression> expressions = new ArrayList<RouteAttributeExpression>();

public void registerSelectValue(String value, String type) {
		if(value.startsWith(":") || value.startsWith("$")) {
			selectValue = value;
		} else {
			selectValue = parseValue(value, type);
			if(selectValue == null) {
				System.err.println("Routing.xml select value '" + value+"' was not registered");
			}
		}
	}

public void printRule(PrintStream out) {
		out.print(" Select " + selectValue + " if ");
		for(int k = 0; k < filterTypes.size(); k++) {
			if(filterTypes.get(k)) {
				String key = universalRulesById.get(k);
				out.print(key + " ");
			}
		}
		if(filterNotTypes.size() > 0) {
			out.print(" ifnot ");
		}
		for(int k = 0; k < filterNotTypes.size(); k++) {
			if(filterNotTypes.get(k)) {
				String key = universalRulesById.get(k);
				out.print(key + " ");
			}
		}
		for(int k = 0; k < parameters.size(); k++) {
			out.print(" param="+parameters.get(k));
		}
		if(onlyTags.size() > 0) {
			out.print(" match tag = " + onlyTags);
		}
		if(onlyNotTags.size() > 0) {
			out.print(" not match tag = " + onlyNotTags);
		}
		out.println();
	}

public void registerAndTagValueCondition(String tag, String value, boolean not) {
		if(value == null) {
			if (not) {
				onlyNotTags.add(tag);
			} else {
				onlyTags.add(tag);
			}
		} else {
			int vtype = registerTagValueAttribute(tag, value);
			if(not) {
				filterNotTypes.set(vtype);
			} else {
				filterTypes.set(vtype);
			}
		}
	}

public void registerLessCondition(String value1, String value2, String valueType) {
		expressions.add(new RouteAttributeExpression(new String[] {value1, value2}, valueType,
						RouteAttributeExpression.LESS_EXPRESSION));
	}

public void registerGreatCondition(String value1, String value2, String valueType) {
		expressions.add(new RouteAttributeExpression(new String[] {value1, value2}, valueType,
						RouteAttributeExpression.GREAT_EXPRESSION));
	}

public void registerAndParamCondition(String param, boolean not) {
		param = not ? "-" + param : param;
		parameters.add(param);
	}

public Object eval(BitSet types, ParameterContext paramContext) {
		if (matches(types, paramContext)) {
			return calcSelectValue(types, paramContext);
		}
		return null;
	}

protected Object calcSelectValue(BitSet types, ParameterContext paramContext) {
		if (selectValue instanceof String && selectValue.toString().startsWith("$")) {
			BitSet mask = tagRuleMask.get(selectValue.toString().substring(1));
			if (mask != null && mask.intersects(types)) {
				BitSet findBit = new BitSet(mask.size());
				findBit.or(mask);
				findBit.and(types);
				int value = findBit.nextSetBit(0);
				return parseValueFromTag(value, selectType);
			}
		} else if (selectValue instanceof String && selectValue.toString().startsWith(":")) {
			String p = ((String) selectValue).substring(1);
			if (paramContext != null && paramContext.vars.containsKey(p)) {
				selectValue = parseValue(paramContext.vars.get(p), selectType);
			} else {
				return null;
			}
		}
		return selectValue;
	}

public boolean matches(BitSet types, ParameterContext paramContext) {
		if(!checkAllTypesShouldBePresent(types)) {
			return false;
		}
		if(!checkAllTypesShouldNotBePresent(types)) {
			return false;
		}
		if(!checkFreeTags(types)) {
			return false;
		}
		if(!checkNotFreeTags(types)) {
			return false;
		}
		if(!checkExpressions(types, paramContext)) {
			return false;
		}
		return true;
	}

private boolean checkExpressions(BitSet types, ParameterContext paramContext) {
		for(RouteAttributeExpression e : expressions) {
			if(!e.matches(types, paramContext)) {
				return false;
			}
		}
		return true;
	}

private boolean checkFreeTags(BitSet types) {
		for (String ts : onlyTags) {
			BitSet b = tagRuleMask.get(ts);
			if (b == null || !b.intersects(types)) {
				return false;
			}
		}
		return true;
	}

private boolean checkNotFreeTags(BitSet types) {
		for (String ts : onlyNotTags) {
			BitSet b = tagRuleMask.get(ts);
			if (b != null && b.intersects(types)) {
				return false;
			}
		}
		return true;
	}

private boolean checkAllTypesShouldNotBePresent(BitSet types) {
		if(filterNotTypes.intersects(types)) {
			return false;
		}
		return true;
	}

private boolean checkAllTypesShouldBePresent(BitSet types) {
		// Bitset method subset is missing "filterTypes.isSubset(types)"
		// reset previous evaluation
		evalFilterTypes.or(filterTypes);
		// evaluate bit intersection and check if filterTypes contained as set in types
		evalFilterTypes.and(types);
		if(!evalFilterTypes.equals(filterTypes)) {
			return false;
		}
		return true;
	}

};
}

RoutingVehicleConfig::~RoutingVehicleConfig() {
}
RoutingVehicleConfig::RoutingVehicleConfig() :
		restrictionsAware(true), minDefaultSpeed(10), maxDefaultSpeed(10), rightTurn(0), leftTurn(0), roundaboutTurn(0) {
	for (int i = 0; i < RouteDataObjectAttribute::LAST - 1; i++) {
		RouteAttributeContext c;
		objectAttributes.push_back(c);
	}
}

RoutingVehicleConfig::RoutingVehicleConfig(RoutingVehicleConfig& parent, QMap<QString, QString>& params) {
	auto it = params.begin();
	for(; it != params.end(); it++) {
		addAttribute(it.key(), it.value());
	}
	for (int i = 0; i < RouteDataObjectAttribute::LAST - 1; i++) {
		RouteAttributeContext c(parent.objectAttributes[i], params);
		objectAttributes.push_back(c);
	}
}

RouteDataObjectAttribute valueOfRouteDataObjectAttribute(QString& s) {
	if (s == "priority") {
		return ROAD_PRIORITIES;
	} else if (s == "speed") {
		return ROAD_SPEED;
	} else if (s == "access") {
		return ACCESS;
	} else if (s == "obstacle_time") {
		return OBSTACLES;
	} else if (s == "obstacle") {
		return ROUTING_OBSTACLES;
	} else if (s == "oneway") {
		return ONEWAY;
	} else {
		ASSERT(0, "Route data object attribute is unkown " << s.toStdString());
		return UKNOWN;
	}
}




void RoutingVehicleConfig::addAttribute(const QString& k,const QString& v) {
	attributes[k] = v;
	if(k=="restrictionsAware") {
		restrictionsAware = parseSilentBoolean(v, restrictionsAware);
	} else if(k=="leftTurn") {
		leftTurn = parseSilentFloat(v, leftTurn);
	} else if(k=="rightTurn") {
		rightTurn = parseSilentFloat(v, rightTurn);
	} else if(k=="roundaboutTurn") {
		roundaboutTurn = parseSilentFloat(v, roundaboutTurn);
	} else if(k=="minDefaultSpeed") {
		minDefaultSpeed = parseSilentFloat(v, minDefaultSpeed * 3.6f) / 3.6f;
	} else if(k=="maxDefaultSpeed") {
		maxDefaultSpeed = parseSilentFloat(v, maxDefaultSpeed * 3.6f) / 3.6f;
	}
}

std::shared_ptr<RouteAttributeContext> RoutingVehicleConfig::getObjContext(RouteDataObjectAttribute a) {
	return objectAttributes[a];
}


void RoutingVehicleConfig::registerBooleanParameter(QString& id, QString& name, QString& description) {
	RoutingParameter rp;
	rp.name = name;
	rp.description = description;
	rp.id = id;
	rp.type = RoutingParameterType::BOOLEAN;
	parameters[rp.id] = rp;

}

void RoutingVehicleConfig::registerNumericParameter(QString& id, QString& name, QString& description,
		QList<double>& numbers, QList<QString> vlsDescriptions) {
	RoutingParameter rp;
	rp.name = name;
	rp.description = description;
	rp.id = id;
	rp.possibleValues = numbers;
	rp.possibleValueDescriptions = vlsDescriptions;
	rp.type = RoutingParameterType::NUMERIC;
	parameters[rp.id] = rp;
}

bool RoutingVehicleConfig::acceptLine(RouteDataObject& way) {
	int res = getObjContext(RouteDataObjectAttribute::ACCESS)->evaluateInt(way, 0);
	return res >= 0;
}

int RoutingVehicleConfig::registerTagValueAttribute(QString& tag, QString& value) {
	QString key = tag +"$"+value;
	if(universalRules.contains(key)) {
		return universalRules[key];
	}
	int id = universalRules.size();
	universalRulesById.add(key);
	universalRules.put(key, id);
	if(!tagRuleMask.containsKey(tag)) {
		tagRuleMask.put(tag, new BitSet());
	}
	tagRuleMask.get(tag).set(id);
	return id;
}


Object parseValue(QString& value, QString& type) {
	float vl = -1;
	value = value.trim();
	if("speed"==type)) {
		vl = RouteDataObject.parseSpeed(value, vl);
	} else if("weight"==type)) {
		vl = RouteDataObject.parseWeightInTon(value, vl);
	} else if("length"==type)) {
		vl = RouteDataObject.parseLength(value, vl);
	} else {
		int i = Algorithms.findFirstNumberEndIndex(value);
		if (i > 0) {
			// could be negative
			return Float.parseFloat(value.substring(0, i));
		}
	}
	if(vl == -1) {
		return null;
	}
	return vl;
}

Object parseValueFromTag(int id, QString type) {
	while (ruleToValue.size() <= id) {
		ruleToValue.add(null);
	}
	Object res = ruleToValue.get(id);
	if (res == null) {
		QString v = universalRulesById.get(id);
		QString value = v.substring(v.indexOf('$') + 1);
		res = parseValue(value, type);
		if (res == null) {
			res = "";
		}
		ruleToValue.set(id, res);
	}
	if (""==res)) {
		return null;
	}
	return res;
}


std::shared_ptr<RoutingVehicleConfig> RoutingVehicleConfig::build(QMap<QString, QString>& params) {
	return std::shared_ptr<RoutingVehicleConfig>(RoutingVehicleConfig(*this, params));
}


float RoutingVehicleConfig::defineObstacle(RouteDataObject road, int point) {
	int[] pointTypes = road.getPointTypes(point);
	if(pointTypes != null) {
		return getObjContext(RouteDataObjectAttribute.OBSTACLES).evaluateFloat(road.region, pointTypes, 0);
	}
	return 0;
}

float RoutingVehicleConfig::defineRoutingObstacle(RouteDataObject road, int point) {
	int[] pointTypes = road.getPointTypes(point);
	if(pointTypes != null){
		return getObjContext(RouteDataObjectAttribute.ROUTING_OBSTACLES).evaluateFloat(road.region, pointTypes, 0);
	}
	return 0;
}

int RoutingVehicleConfig::isOneWay(RouteDataObject& road) {
	return getObjContext(RouteDataObjectAttribute::ONEWAY).evaluateInt(road, 0);
}

float RoutingVehicleConfig::defineSpeed(RouteDataObject& road) {
	return getObjContext(RouteDataObjectAttribute::ROAD_SPEED) .evaluateFloat(road, getMinDefaultSpeed() * 3.6f) / 3.6f;
}

float RoutingVehicleConfig::defineSpeedPriority(RouteDataObject& road) {
	return getObjContext(RouteDataObjectAttribute::ROAD_PRIORITIES).evaluateFloat(road, 1f);
}


double RoutingVehicleConfig::calculateTurnTime(RouteSegment segment, int segmentEnd, RouteSegment prev, int prevSegmentEnd) {
	int[] pt = prev.getRoad().getPointTypes(prevSegmentEnd);
	if(pt != null) {
		RouteRegion reg = prev.getRoad().region;
		for(int i=0; i<pt.length; i++) {
			RouteTypeRule r = reg.quickGetEncodingRule(pt[i]);
			if("highway"==r.getTag()) && "traffic_signals"==r.getValue())) {
				// traffic signals don't add turn info
				return 0;
			}
		}
	}
	double rt = roundaboutTurn;
	if(rt > 0 && !prev.getRoad().roundabout() && segment.getRoad().roundabout()) {
		return rt;
	}
	if (leftTurn > 0 || rightTurn > 0) {
		double a1 = segment.getRoad().directionRoute(segment.getSegmentStart(), segment.getSegmentStart() < segmentEnd);
		double a2 = prev.getRoad().directionRoute(prevSegmentEnd, prevSegmentEnd < prev.getSegmentStart());
		double diff = Math.abs(MapUtils.alignAngleDifference(a1 - a2 - Math.PI));
		// more like UT
		if (diff > 2 * PI / 3) {
			return leftTurn;
		} else if (diff > PI / 2) {
			return rightTurn;
		}
		return 0;
	}
	return 0;
}


bool RoutingVehicleConfig::containsAttribute(QString& attribute) {
	return attributes.contains(attribute);
}

QString& RoutingVehicleConfig::getAttribute(QString& attribute) {
	return attributes[attribute];
}




class RouteAttributeContext {
QList<std::shared_ptr<RouteAttributeEvalRule> > rules;
QMap<QString, QString> paramContext;
private:
Object RouteAttributeContext::evaluate(RouteDataObject& ro) {
	return evaluate(convert(ro.region, ro.types));
}

bool RouteAttributeContext::checkParameter(RouteAttributeEvalRule& r) {
	if (!paramContext.isEmpty()  && r.parameters.size() > 0) {
		for (QString p : r.parameters) {
			bool nt = false;
			if (p.startsWith("-")) {
				nt = true;
				p = p.substring(1);
			}
			bool val = paramContext.vars.containsKey(p);
			if (nt && val) {
				return false;
			} else if (!not && !val) {
				return false;
			}
		}
	}
	return true;
}


std::shared_ptr<RouteAttributeEvalRule> RouteAttributeContext::registerNewRule(QString& selectValue, QString& selectType) {
	std::shared_ptr<RouteAttributeEvalRule> ev(new RouteAttributeEvalRule);
	ev.registerSelectValue(selectValue, selectType);
	rules.push_back(ev);
	return ev;
}

std::shared_ptr<RouteAttributeEvalRule> RouteAttributeContext::getLastRule() {
	return (rules.end()--);
}

Object RouteAttributeContext::evaluate(QBitArray& types) {
	for (int k = 0; k < rules.size(); k++) {
		RouteAttributeEvalRule r = rules.get(k);
		Object o = r.eval(types, paramContext);
		if (o != null) {
			return o;
		}
	}
	return null;
}

public:

RouteAttributeContext(RouteAttributeContext& cp, QMap<QString, QString>& params) {
	paramContext = params;
	for (RouteAttributeEvalRule rt : cp.rules) {
		if (checkParameter(rt)) {
			rules.push_back(rt);
		}
	}
}

std::shared_ptr<RouteAttributeEvalRule> getLastRule();
void registerNewRule(QString val, QString type);
std::shared_ptr<RouteAttributeEvalRule> registerNewRule(QString& selectValue, QString& selectType);
Object evaluate(QBitArray& types);

int evaluateInt(RouteDataObject& ro, int defValue);
float evaluateFloat(RouteDataObject& ro, float defValue);
};


y
int RouteAttributeContext::evaluateInt(RouteDataObject& ro, int defValue) {
	Object o = evaluate(ro);
	if(!(o instanceof Number)) {
		return defValue;
	}
	return ((Number)o).intValue();
}

int RouteAttributeContext::evaluateInt(RouteRegion region, int[] types, int defValue) {
	Object o = evaluate(convert(region, types));
	if(!(o instanceof Number)){
		return defValue;
	}
	return ((Number)o).intValue();
}

float RouteAttributeContext::evaluateFloat(RouteDataObject& ro, float defValue) {
	Object o = evaluate(ro);
	if(!(o instanceof Number)) {
		return defValue;
	}
	return ((Number)o).floatValue();
}

float RouteAttributeContext::evaluateFloat(RouteRegion& region, int[] types, float defValue) {
	Object o = evaluate(convert(region, types));
	if(!(o instanceof Number)) {
		return defValue;
	}
	return ((Number)o).floatValue();
}

QBitArray RouteAttributeContext::convert(RouteRegion reg, int[] types) {
	BitSet b = new BitSet(universalRules.size());
	Map<Integer, Integer> map = regionConvert.get(reg);
	if(map == null){
		map = new HashMap<Integer, Integer>();
		regionConvert.put(reg, map);
	}
	for(int k = 0; k < types.length; k++) {
		Integer nid = map.get(types[k]);
		if(nid == null){
			RouteTypeRule r = reg.quickGetEncodingRule(types[k]);
			nid = registerTagValueAttribute(r.getTag(), r.getValue());
			map.put(types[k], nid);
		}
		b.set(nid);
	}
	return b;
}



} /* namespace OsmAnd */
