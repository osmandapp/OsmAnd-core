#ifndef _OSMAND_ROUTE_SEGMENT_RESULT_CPP
#define _OSMAND_ROUTE_SEGMENT_RESULT_CPP
#include "routeSegmentResult.h"

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

void RouteSegmentResult::fillNames(SHARED_PTR<RouteDataResources> resources) {
    if (!object->namesIds.empty()) {
        RoutingIndex *region = object->region;
        int nameTypeRule = region->nameTypeRule;
        int refTypeRule = region->refTypeRule;
        object->names.clear();
        for (pair<uint32_t, uint32_t> nameIdPair : object->namesIds) {
            uint32_t nameId = nameIdPair.second;
            RouteTypeRule rule = region->quickGetEncodingRule(nameId);
            //if (rule != NULL) {
            if (nameTypeRule != -1 && "name" == rule.getTag()) {
                nameId = nameTypeRule;
            } else if (refTypeRule != -1 && "ref" == rule.getTag()) {
                nameId = refTypeRule;
            }
            auto n = object->names;
            object->names.insert({nameId, rule.getValue()});
            //}
        }
    }
    std::vector<std::vector<string>> pointNames;
    std::vector<std::vector<uint32_t>> pointNameTypes;
    pointNames.clear();
    pointNameTypes.clear();
    std::vector<std::vector<uint32_t>> pointNamesArr = resources->getPointNamesMap()[object];
    //if (pointNamesArr != NULL) {
    pointNames = std::vector<std::vector<string>>(pointNamesArr.size(), std::vector<string>());
    pointNameTypes = std::vector<std::vector<uint32_t>>(pointNamesArr.size(), std::vector<uint32_t>());
    for (int i = 0; i < pointNamesArr.size(); i++) {
        std::vector<uint32_t> namesIds = pointNamesArr[i];
        //if (namesIds != NULL) {
        pointNames[i] = std::vector<string>(namesIds.size());
        pointNameTypes[i] = std::vector<uint32_t>(namesIds.size());
        for (int k = 0; k < namesIds.size(); k++) {
            int id = namesIds[k];
            RouteTypeRule r = object->region->quickGetEncodingRule(id);
            //if (r != NULL) {
            pointNames[i][k] = r.getValue();
            int nameType = object->region->searchRouteEncodingRule(r.getTag(), "");
            if (nameType != -1) {
                pointNameTypes[i][k] = nameType;
            }
            //}
        }
        //}
    }
    //}
    
    object->pointNames = pointNames;
    object->pointNameTypes = pointNameTypes;
}

#endif /*_OSMAND_ROUTE_SEGMENT_RESULT_CPP*/
