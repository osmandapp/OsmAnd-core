#ifndef _OSMAND_GENERAL_ROUTER_CPP
#define _OSMAND_GENERAL_ROUTER_CPP
#include "generalRouter.h"
const int RouteAttributeExpression::LESS_EXPRESSION = 1;
const int RouteAttributeExpression::GREAT_EXPRESSION = 1;



double GeneralRouter::parseValue(string value, string type) {
	double vl = -1;
	if("speed" == type) {
		vl = RouteDataObject::parseSpeed(value, vl);
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

double GeneralRouter::parseValueFromTag(uint id, string type, GeneralRouter& router) {
	while (ruleToValue.size() <= id) {
		ruleToValue.push_back(DOUBLE_MISSING);
	}
	double res = ruleToValue[id];
	if (res == DOUBLE_MISSING) {
		tag_value v = router.universalRulesById[id];
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
	return res >= 0;
}

uint GeneralRouter::registerTagValueAttribute(const tag_value& r) {
	string key = r.first + "$" + r.second;
	MAP_STR_INT::iterator it = universalRules.find(key);
	if(it != universalRules.end()) {
		return ((uint)it->second);
	}
	uint id = universalRules.size();
	universalRulesById.push_back(r);
	universalRules[key] = id;
	tagRuleMask[r.first].set(id);
	return id;
}

dynbitset RouteAttributeContext::convert(RoutingIndex* reg, std::vector<uint32_t>& types) {
	dynbitset b(router.universalRules.size());
	MAP_INT_INT map = router.regionConvert[reg];
	for(uint k = 0; k < types.size(); k++) {
		MAP_INT_INT::iterator nid = map.find(types[k]);
		int vl;
		if(nid == map.end()){
			tag_value r = reg->decodingRules[types[k]];
			vl = router.registerTagValueAttribute(r);
			map[types[k]] = vl;
		} else {
			vl = nid->second;
		}
		b.set(vl);
	}
	return b;
}

double RouteAttributeEvalRule::eval(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	if (matches(types, paramContext, router)) {
		return calcSelectValue(types, paramContext, router);
	}
	return DOUBLE_MISSING;
}


double RouteAttributeEvalRule::calcSelectValue(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	if (selectValueParam.length() > 0 && selectValueParam.find("$", 0) == 0) {
		UNORDERED(map)<string, dynbitset >::iterator ms = router.tagRuleMask.find(selectValueParam.substr(1));
		if (ms != router.tagRuleMask.end() && ms->second.intersects(types)) {
			dynbitset findBit(ms->second.size());
			findBit |= ms->second;
			findBit &= types;
			uint value = findBit.find_first();
			return router.parseValueFromTag(value, selectType, router);
		}
	} else if (selectValueParam.length() > 0 && selectValueParam.find(":", 0) == 0) {
		string p = selectValueParam.substr(1);
		MAP_STR_STR::iterator it = paramContext.vars.find(p);
		if (it != paramContext.vars.end()) {
			selectValue = router.parseValue(it->second, selectType);
		} else {
			return DOUBLE_MISSING;
		}
	}
	return selectValue;
}

bool RouteAttributeExpression::matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	double f1 = calculateExprValue(0, types, paramContext, router);
	double f2 = calculateExprValue(1, types, paramContext, router);
	if(f1 == DOUBLE_MISSING || f2 == DOUBLE_MISSING) {
		return false;
	}
	if (expressionType == LESS_EXPRESSION) {
		return f1 <= f2;
	} else if (expressionType == GREAT_EXPRESSION) {
		return f1 >= f2;
	}
	return false;
}

/** TODO
double calculateExprValue(int id, dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
			String value = values[id];
			TODO;
			double cacheValue = cacheValues[id];
			if(cacheValue != null) {
				return cacheValue.doubleValue();
			}
			double o = DOUBLE_MISSING;
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
			return o;
		} */

bool RouteAttributeEvalRule::matches(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
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

bool RouteAttributeEvalRule::checkExpressions(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	for(uint i = 0; i < expressions.size(); i++) {
		if(!expressions[i].matches(types, paramContext, router)) {
			return false;
		}
	}
	return true;
}

bool RouteAttributeEvalRule::checkFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	for(UNORDERED(set)<string>::iterator it = onlyTags.begin(); it != onlyTags.end(); it++) {
		UNORDERED(map)<string, dynbitset >::iterator ms = router.tagRuleMask.find(*it);
		if (ms == router.tagRuleMask.end() || !ms->second.intersects(types)) {
			return false;
		}
	}
	return true;
}

bool RouteAttributeEvalRule::checkNotFreeTags(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	for(UNORDERED(set)<string>::iterator it = onlyNotTags.begin(); it != onlyNotTags.end(); it++) {
		UNORDERED(map)<string, dynbitset >::iterator ms = router.tagRuleMask.find(*it);
		if (ms != router.tagRuleMask.end() && ms->second.intersects(types)) {
			return false;
		}
	}
	return true;
}

bool RouteAttributeEvalRule::checkAllTypesShouldNotBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	if(filterNotTypes.intersects(types)) {
		return false;
	}
	return true;
}

bool RouteAttributeEvalRule::checkAllTypesShouldBePresent(dynbitset& types, ParameterContext& paramContext, GeneralRouter& router) {
	// Bitset method subset is missing "filterTypes.isSubset(types)"    
	// reset previous evaluation
	return filterTypes.is_subset_of(types);
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