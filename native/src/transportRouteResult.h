#ifndef _OSMAND_TRANSPORT_ROUTE_RESULT_H
#define _OSMAND_TRANSPORT_ROUTE_RESULT_H
#include "transportRouteResultSegment.h"
#include "transportRoutingConfiguration.h"
#include "transportRoutingContext.h"
 

struct TransportRouteResult {
    vector<SHARED_PTR<TransportRouteResultSegment>> segments;
    double finishWalkDist;
    double routeTime;
    SHARED_PTR<TransportRoutingConfiguration> config;

    TransportRouteResult(TransportRoutingContext* ctx) {
        config = ctx->cfg;
    }
    
    //ui/logging
    double getWalkDist() {
        double d = finishWalkDist;
        for (vector<SHARED_PTR<TransportRouteResultSegment>>::iterator it = segments.begin(); it != segments.end(); it++) {
            d += (*it)->walkDist;
        }
        return d;
    }

    float getWalkSpeed() {
        return config->walkSpeed;
    }
    
    //logging only
    int getStops() {
        int stops = 0;
        for (vector<SHARED_PTR<TransportRouteResultSegment>>::iterator it = segments.begin(); it != segments.end(); it++) {
            stops += ((*it)->end - (*it)->start);
        }
        return stops;
    }

    // ui only:
    // bool isRouteStop (TransportStop stop) {
    //     for (vector<TransportRouteResultSegment>::iterator it = segments.begin(); it != segments.end(); it++) {
    //         if (find(*it->getTravelStops().begin(), *it->getTravelStops().end(), stop) != *it->getTravelStops().end()) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }

    //for ui/logs
    double getTravelDist() {
        double d = 0;
        for (SHARED_PTR<TransportRouteResultSegment>& it : segments) {
            d += it->getTravelDist();
        }
        return d;
    }

    //for ui/logs
    double getTravelTime() {
        double t = 0;
        for (SHARED_PTR<TransportRouteResultSegment> seg : segments) {
            if (config->useSchedule) {
                SHARED_PTR<TransportSchedule> sts = seg->route->schedule;
                for (int k = seg->start; k < seg->end; k++) {
                    t += sts->avgStopIntervals[k] * 10;
                }
            } else {
                t += config->getBoardingTime();
                t += seg->travelTime;
            }
        }
        return t;
    }

    //for ui/logs
    double getWalkTime() {
        return getWalkDist() / config->walkSpeed;
    }
    //for ui/logs
    double getChangeTime() {
        return config->changeTime;
    }
    //for ui/logs
    double getBoardingTime() {
        return config->boardingTime;
    }
    //for ui/logs
    int getChanges() {
        return segments.size() - 1;
    }

    string to_string() {
        //todo add logs
        return "";
    }
};

#endif /*_OSMAND_TRANSPORT_ROUTE_RESULT_H*/
