#include "routeTypeRule.h"
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

float RouteTypeRule::maxSpeed() {
    if (type == MAXSPEED) {
        if (!conditions.empty()) {
            for (const auto& c : conditions) {
                if (c.hours != nullptr && c.hours->isOpened()) {
                    return c.floatValue;
                }
            }
        }
        return floatValue;
    }
    return -1;
}

void RouteTypeRule::analyze() {
    std::string tl = to_lowercase(t);
    if (tl == "oneway") {
        type = ONEWAY;
        if("-1" == v || "reverse" == v) {
            intValue = -1;
        } else if ("1" == v || "yes" == v) {
            intValue = 1;
        } else {
            intValue = 0;
        }
    } else if (tl == "highway" && "traffic_signals" == v) {
        type = TRAFFIC_SIGNALS;
    } else if (tl == "railway" && ("crossing" == v || "level_crossing" == v)) {
        type = RAILWAY_CROSSING;
    } else if (tl == "roundabout" && !v.empty()) {
        type = ROUNDABOUT;
    } else if (tl == "junction" && "roundabout" == to_lowercase(v)) {
        type = ROUNDABOUT;
    } else if (tl == "highway" && !v.empty()) {
        type = HIGHWAY_TYPE;
    } else if (startsWith(t, "access") && !v.empty()) {
        type = ACCESS;
    } else if (tl == "maxspeed:conditional" && !v.empty()) {
        conditions.clear();
        std::vector<std::string> cts = split_string(v, ";");
        for(auto& c : cts) {
            auto ch = c.find("@");
            if (ch != std::string::npos) {
                RouteTypeCondition cond;
                cond.floatValue = parseSpeed(c.substr(0, ch), 0);
                cond.condition = trim(c.substr(ch + 1));
                if (startsWith(cond.condition, "(")) {
                    cond.condition = trim(cond.condition.substr(1, cond.condition.length() - 1));
                }
                if(endsWith(cond.condition, ")")) {
                    cond.condition = trim(cond.condition.substr(0, cond.condition.length() - 1));
                }
                cond.hours = OpeningHoursParser::parseOpenedHours(cond.condition);
                conditions.push_back(cond);
            }
        }
        type = MAXSPEED;
    } else if (tl == "maxspeed" && !v.empty()){
        type = MAXSPEED;
        floatValue = parseSpeed(v, 0);
    } else if (tl == "maxspeed:forward" && !v.empty()) {
        type = MAXSPEED;
        forward = 1;
        floatValue = parseSpeed(v, 0);
    } else if (tl == "maxspeed:backward" && !v.empty()) {
        type = MAXSPEED;
        forward = -1;
        floatValue = parseSpeed(v, 0);
    } else if (tl == "lanes" && !v.empty()) {
        intValue = -1;
        int i = 0;
        type = LANES;
        while (i < v.length() && isdigit(v[i])) {
            i++;
        }
        if (i > 0) {
            int val = 0;
            if (sscanf(v.substr(0, i).c_str(), "%d", &val) != EOF) {
                intValue = val;
            } else {
                intValue = 0;
            }
        }
    } else if (tl == "tunnel" && !v.empty()) {
        type = TUNNEL;
    }
}
