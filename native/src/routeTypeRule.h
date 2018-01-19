#ifndef _OSMAND_ROUTE_TYPE_RULE_H_
#define _OSMAND_ROUTE_TYPE_RULE_H_

#include <string>
#include <vector>

#include "openingHoursParser.h"

struct RouteTypeCondition {
    std::string condition;
    std::shared_ptr<OpeningHoursParser::OpeningHours> hours;
    float floatValue;
    
    RouteTypeCondition() : condition(""), hours(nullptr), floatValue(0) {
    }
};

struct RouteTypeRule {
    const static int ACCESS = 1;
    const static int ONEWAY = 2;
    const static int HIGHWAY_TYPE = 3;
    const static int MAXSPEED = 4;
    const static int ROUNDABOUT = 5;
    const static int TRAFFIC_SIGNALS = 6;
    const static int RAILWAY_CROSSING = 7;
    const static int LANES = 8;
    
private:
    std::string t;
    std::string v;
    int intValue;
    float floatValue;
    int type;
    std::vector<RouteTypeCondition> conditions;
    int forward;
    
private:
    void analyze();
    
public:
    RouteTypeRule() : t(""), v("") {
    }
    
    RouteTypeRule(std::string t, std::string v) : t(t) {
        if ("true" == v) {
            v = "yes";
        }
        if ("false" == v) {
            v = "no";
        }
        this->v = v;
        this->analyze();
    }
    
    inline int isForward() {
        return forward;
    }
    
    inline const std::string& getTag() {
        return t;
    }
    
    inline const std::string& getValue() {
        return v;
    }
    
    inline bool roundabout() {
        return type == ROUNDABOUT;
    }
    
    inline int getType() {
        return type;
    }
    
    inline bool conditional() {
        return !conditions.empty();
    }
    
    inline int onewayDirection() {
        if (type == ONEWAY){
            return intValue;
        }
        return 0;
    }
    
    float maxSpeed();
    
    inline int lanes() {
        if (type == LANES) {
            return intValue;
        }
        return -1;
    }
    
    inline std::string highwayRoad() {
        if (type == HIGHWAY_TYPE) {
            return v;
        }
        return "";
    }
};

#endif
