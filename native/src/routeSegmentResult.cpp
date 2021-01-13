#ifndef _OSMAND_ROUTE_SEGMENT_RESULT_CPP
#define _OSMAND_ROUTE_SEGMENT_RESULT_CPP
#include "routeSegmentResult.h"

// RouteDataBundle
RouteDataBundle::RouteDataBundle(SHARED_PTR<RouteDataResources>& resources)
    :resources(resources)
{
}

void RouteDataBundle::put(std::string key, std::string value) {
    data[key] = value;
}

void RouteDataBundle::putVector(std::string key, std::vector<uint32_t> value) {
    put(key, vectorToString(value));
}

void RouteDataBundle::putVectors(std::string key, std::vector<std::vector<uint32_t>> value) {
    put(key, vectorArrayToString(value));
}

std::string RouteDataBundle::getString(string key) {
    return data[key];
}

std::string RouteDataBundle::vectorToString(std::vector<uint32_t>& vec) {
    std::ostringstream oss;
    if (!vec.empty())
    {
        if (vec.size() > 1) {
            std::copy(vec.begin(), vec.end() - 1,
                      std::ostream_iterator<uint32_t>(oss, ";"));
        }
        oss << vec.back();
    }
    return oss.str();
}

std::string RouteDataBundle::vectorArrayToString(std::vector<std::vector<uint32_t>>& vec) {
    string res;
    if (!vec.empty())
    {
        for (auto it = vec.begin(); vec.size() > 1 && it != vec.end() - 1; ++it) {
            res += vectorToString(*it);
            res += ";";
        }
        res += vectorToString(vec.back());
    }
    return res;
}

// RouteSegmentResult

void RouteSegmentResult::collectTypes(SHARED_PTR<RouteDataResources>& resources)
{
    auto &rules = resources->rules;
    if (object->types.size() > 0) {
        collectRules(rules, object->types);
    }
    if (object->pointTypes.size() > 0) {
        int start = min(startPointIndex, endPointIndex);
        int end = max(startPointIndex, endPointIndex);
        for (int i = start; i <= end && i < object->pointTypes.size(); i++) {
            vector<uint32_t> types = object->pointTypes[i];
            if (types.size() > 0) {
                collectRules(rules, types);
            }
        }
    }
}

void RouteSegmentResult::collectNames(SHARED_PTR<RouteDataResources>& resources) {
    auto& rules = resources->rules;
    RoutingIndex *region = object->region;
    if (region->nameTypeRule != -1) {
        auto& r = region->quickGetEncodingRule(region->nameTypeRule);
        if (rules.find(r) == rules.end()) {
            rules[r] = (uint32_t) rules.size();
        }
    }
    if (region->refTypeRule != -1) {
        auto& r = region->quickGetEncodingRule(region->refTypeRule);
        if (rules.find(r) == rules.end()) {
            rules[r] = (uint32_t) rules.size();
        }
    }
    if (object->namesIds.size() > 0) {
        for (const auto& nameId : object->namesIds) {
            const auto& name = object->names[nameId.first];
            const auto& tag = region->quickGetEncodingRule(nameId.first).getTag();
            RouteTypeRule r(tag, name);
            if (rules.find(r) == rules.end()) {
                rules[r] = (uint32_t) rules.size();
            }
        }
    }
    if (object->pointNameTypes.size() > 0) {
        int start = min(startPointIndex, endPointIndex);
        int end = min((int) max(startPointIndex, endPointIndex) + 1, (int) object->pointNameTypes.size());
        for (int i = start; i < end; i++) {
            auto& types = object->pointNameTypes[i];
            if (types.size() > 0) {
                for (uint32_t type : types) {
                    auto& r = region->quickGetEncodingRule(type);
                    if (rules.find(r) == rules.end()) {
                        rules[r] = (uint32_t) rules.size();
                    }
                }
            }
        }
    }
}

void RouteSegmentResult::collectRules(UNORDERED_map<RouteTypeRule, uint32_t>& rules, vector<uint32_t>& types) {
    RoutingIndex *region = object->region;
    for (uint32_t type : types)
    {
        auto& rule = region->quickGetEncodingRule(type);
        const auto& tag = rule.getTag();
        if (tag == "osmand_ele_start" || tag == "osmand_ele_end"
            || tag == "osmand_ele_asc" || tag == "osmand_ele_desc")
        {
            continue;
        }
        if (rules.find(rule) == rules.end()) {
            rules[rule] = (uint32_t) rules.size();
        }
    }
}

std::vector<uint32_t> RouteSegmentResult::convertTypes(std::vector<uint32_t>& types, UNORDERED_map<RouteTypeRule, uint32_t>& rules) {
    vector<uint32_t> arr;
    if (types.size() == 0) {
        return arr;
    }
    for (int i = 0; i < types.size(); i++) {
        uint32_t type = types[i];
        auto& rule = object->region->quickGetEncodingRule(type);
        uint32_t ruleId = rules[rule];
        if (ruleId) {
            arr.push_back(ruleId);
        }
    }
    vector<uint32_t> res(arr.size());
    for (int i = 0; i < arr.size(); i++) {
        res[i] = arr[i];
    }
    return res;
}

std::vector<std::vector<uint32_t>> RouteSegmentResult::convertTypes(std::vector<std::vector<uint32_t>> &types, UNORDERED_map<RouteTypeRule, uint32_t>& rules) {
    vector<vector<uint32_t>> res(types.size());
    if (types.size() == 0) {
        return res;
    }
    for (int i = 0; i < types.size(); i++) {
        auto typesArr = types[i];
        if (typesArr.size() > 0) {
            res[i] = convertTypes(typesArr, rules);
        }
    }
    return res;
}

std::vector<uint32_t> RouteSegmentResult::convertNameIds(vector<pair<uint32_t, uint32_t> >& nameIds, UNORDERED_map<RouteTypeRule, uint32_t>& rules) {
    vector<uint32_t> res(nameIds.size());
    if (nameIds.size() == 0) {
        return res;
    }
    for (int i = 0; i < nameIds.size(); i++) {
        uint32_t nameId = nameIds[i].first;
        auto& name = object->names[nameId];
        auto& tag = object->region->quickGetEncodingRule(nameId).getTag();
        RouteTypeRule rule(tag, name);
        uint32_t ruleId = rules[rule];
        if (rules.find(rule) == rules.end()) {
            res.clear();
            return res;
        }
        res[i] = ruleId;
    }
    return res;
}

std::vector<std::vector<uint32_t>> RouteSegmentResult::convertPointNames(std::vector<std::vector<uint32_t>>& nameTypes, std::vector<std::vector<std::string>>& pointNames, UNORDERED_map<RouteTypeRule, uint32_t>& rules) {
    std::vector<std::vector<uint32_t>> res(nameTypes.size());
    if (nameTypes.size() == 0) {
        return res;
    }
    for (int i = 0; i < nameTypes.size(); i++) {
        auto types = nameTypes[i];
        if (types.size() > 0) {
            vector<uint32_t> arr(types.size());
            for (int k = 0; k < types.size(); k++) {
                uint32_t type = types[k];
                auto& tag = object->region->quickGetEncodingRule(type).getTag();
                auto& name = pointNames[i][k];
                RouteTypeRule rule(tag, name);
                const auto it = rules.find(rule);
                uint32_t ruleId;
                if (it == rules.end()) {
                    ruleId = (uint32_t) rules.size();
                    rules[rule] = ruleId;
                }
                else {
                    ruleId = it->second;
                }
                arr[k] = ruleId;
            }
            res[i] = arr;
        }
    }
    return res;
}
// TODO: port gpx reading for route planning
//public void fillNames(RouteDataResources resources) {
//    if (object.nameIds != null && object.nameIds.length > 0) {
//        RouteRegion region = object.region;
//        int nameTypeRule = region.getNameTypeRule();
//        int refTypeRule = region.getRefTypeRule();
//        object.names = new TIntObjectHashMap<>();
//        for (int nameId : object.nameIds) {
//            RouteTypeRule rule = region.quickGetEncodingRule(nameId);
//            if (rule != null) {
//                if (nameTypeRule != -1 && "name".equals(rule.getTag())) {
//                    nameId = nameTypeRule;
//                } else if (refTypeRule != -1 && "ref".equals(rule.getTag())) {
//                    nameId = refTypeRule;
//                }
//                object.names.put(nameId, rule.getValue());
//            }
//        }
//    }
//    String[][] pointNames = null;
//    int[][] pointNameTypes = null;
//    int[][] pointNamesArr = resources.getPointNamesMap().get(object);
//    if (pointNamesArr != null) {
//        pointNames = new String[pointNamesArr.length][];
//        pointNameTypes = new int[pointNamesArr.length][];
//        for (int i = 0; i < pointNamesArr.length; i++) {
//            int[] namesIds = pointNamesArr[i];
//            if (namesIds != null) {
//                pointNames[i] = new String[namesIds.length];
//                pointNameTypes[i] = new int[namesIds.length];
//                for (int k = 0; k < namesIds.length; k++) {
//                    int id = namesIds[k];
//                    RouteTypeRule r = object.region.quickGetEncodingRule(id);
//                    if (r != null) {
//                        pointNames[i][k] = r.getValue();
//                        int nameType = object.region.searchRouteEncodingRule(r.getTag(), null);
//                        if (nameType != -1) {
//                            pointNameTypes[i][k] = nameType;
//                        }
//                    }
//                }
//            }
//        }
//    }
//    object.pointNames = pointNames;
//    object.pointNameTypes = pointNameTypes;
//}

void RouteSegmentResult::writeToBundle(SHARED_PTR<RouteDataBundle>& bundle) {
    auto& rules = bundle->resources->rules;
    bool reversed = endPointIndex < startPointIndex;
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << segmentTime;
    bundle->put("length", to_string(abs(endPointIndex - startPointIndex) + 1));
    bundle->put("segmentTime", ss.str());
    ss.str("");
    ss << std::fixed << std::setprecision(2) << segmentSpeed;
    bundle->put("speed", ss.str());
    ss.str("");
    if (turnType) {
        bundle->put("turnType", turnType->toXmlString());
        if (turnType->isSkipToSpeak()) {
            bundle->put("skipTurn", turnType->isSkipToSpeak() ? string("true") : string("false"));
        }
        if (turnType->getTurnAngle() != 0) {
            ss << std::fixed << std::setprecision(2) << turnType->getTurnAngle();
            bundle->put("turnAngle", ss.str());
            ss.str("");
        }
        auto& turnLanes = turnType->getLanes();
        if (turnLanes.size() > 0) {
            bundle->put("turnLanes", TurnType::toString(turnLanes));
        }
    }
    bundle->put("id", to_string(object->id >> 6)); // OsmAnd ID to OSM ID
    bundle->putVector("types", convertTypes(object->types, rules));
    
    int start = min(startPointIndex, endPointIndex);
    int end = max(startPointIndex, endPointIndex) + 1;
    if (start < object->pointTypes.size()) {
        vector<vector<uint32_t>> types(min(end, (int) object->pointTypes.size()) - start);
        copy(object->pointTypes.begin() + start, object->pointTypes.begin() + min(end, (int) object->pointTypes.size()), types.begin());
        if (reversed) {
            reverse(types.begin(), types.end());
        }
        bundle->putVectors("pointTypes", convertTypes(types, rules));
    }
    bundle->putVector("names", convertNameIds(object->namesIds, rules));
    if (start < object->pointNameTypes.size())
    {
        vector<vector<uint32_t>> types(min(end, (int) object->pointNameTypes.size()) - start);
        copy(object->pointNameTypes.begin() + start, object->pointNameTypes.begin() + min(end, (int) object->pointNameTypes.size()), types.begin());
        vector<vector<string>> names(min(end, (int) object->pointNames.size()) - start);
        copy(object->pointNames.begin() + start, object->pointNames.begin() + min(end, (int) object->pointNames.size()), names.begin());
        if (reversed) {
            reverse(types.begin(), types.end());
            reverse(names.begin(), names.end());
        }
        bundle->putVectors("pointNames", convertPointNames(types, names, rules));
    }
}

void RouteSegmentResult::attachRoute(int roadIndex, SHARED_PTR<RouteSegmentResult> r) {
    if (r->object->isRoadDeleted()) {
        return;
    }
    int st = abs(roadIndex - startPointIndex);
    if (st >= attachedRoutesFrontEnd.size()) {
        attachedRoutesFrontEnd.resize(st + 1);
    }
    attachedRoutesFrontEnd[st].push_back(r);
}

void RouteSegmentResult::copyPreattachedRoutes(SHARED_PTR<RouteSegmentResult> toCopy, int shift) {
    if (!toCopy->preAttachedRoutes.empty()) {
        preAttachedRoutes.clear();
        preAttachedRoutes.insert(preAttachedRoutes.end(), toCopy->preAttachedRoutes.begin() + shift, toCopy->preAttachedRoutes.end());
    }
}

vector<SHARED_PTR<RouteSegmentResult> > RouteSegmentResult::getPreAttachedRoutes(int routeInd) {
    int st = abs(routeInd - startPointIndex);
    if (!preAttachedRoutes.empty() && st < preAttachedRoutes.size()) {
        return preAttachedRoutes[st];
    }
    return vector<SHARED_PTR<RouteSegmentResult> >();
}

vector<SHARED_PTR<RouteSegmentResult> > RouteSegmentResult::getAttachedRoutes(int routeInd) {
    int st = abs(routeInd - startPointIndex);
    if (st >= attachedRoutesFrontEnd.size()) {
        return vector<SHARED_PTR<RouteSegmentResult> >();
    } else {
        return attachedRoutesFrontEnd[st];
    }
}

// RouteDataResources
RouteDataResources::RouteDataResources(std::vector<std::pair<double, double>> locations)
    : locations(locations)
{
}

pair<double, double> RouteDataResources::getLocation(int index) {
    index += currentLocation;
    if (index < locations.size()) {
        return locations[index];
    }
    return {NAN, NAN};
}
void RouteDataResources::incrementCurrentLocation(int index)
{
    currentLocation += index;
}

#endif /*_OSMAND_ROUTE_SEGMENT_RESULT_CPP*/
