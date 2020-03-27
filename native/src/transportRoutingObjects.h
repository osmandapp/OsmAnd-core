#ifndef _OSMAND_TRANSPORT_ROUTING_OBJECTS_H
#define _OSMAND_TRANSPORT_ROUTING_OBJECTS_H
#include "Logging.h"
#include "commonOsmAndCore.h"
#include <algorithm>


const static int TRANSPORT_STOP_ZOOM = 24;

struct MapObject {
    int64_t id;
    double lat;
    double lon;
    string name;
    string enName;
    map<string,string> names;

    int32_t fileOffset;

    map<string, string> getNamesMap (bool includeEn) {
        if (!includeEn || enName.size() == 0) {
            if (names.size()== 0) {
                return map<string, string>();
            }
            return names;
        }
        map<string, string> tmp;
        tmp.insert(names.begin(), names.end());
        names.insert({"en", enName});
        return names;
    }

    string getName(string lang)
    {
        if (names.find(lang) != names.end())
            return names[lang];
        return "";
    }

};

struct TransportStopExit {
    int x31;
    int y31;
    string ref;

    void setLocation(int zoom, int32_t dx, int32_t dy) {
        x31 = dx << (31 - zoom);
        y31 = dy << (31 - zoom);
        // setLocation(MapUtils.getLatitudeFromTile(zoom, dy), MapUtils.getLongitudeFromTile(zoom, dx));
    }

    bool compareExit(SHARED_PTR<TransportStopExit>& thatObj) {
        return x31 == thatObj->x31 && y31 == thatObj->y31 && ref == thatObj->ref;
    }

};

struct TransportRoute;

struct TransportStop : public MapObject {
    const static int32_t DELETED_STOP = -1;

    vector<int32_t> referencesToRoutes;
    vector<int64_t> deletedRoutesIds;
    vector<int64_t> routesIds;
    int32_t distance;
    int32_t x31;
    int32_t y31;
    vector<SHARED_PTR<TransportStopExit>> exits;
    vector<SHARED_PTR<TransportRoute>> routes;
    map<string, vector<int32_t>> referencesToRoutesMap; //add linked realizations?
    
    bool isDeleted() {
        return referencesToRoutes.size() == 1 && referencesToRoutes[0] ==  DELETED_STOP;
    }

    bool isRouteDeleted(int64_t routeId) {
        return std::find(deletedRoutesIds.begin(), deletedRoutesIds.end(), routeId) != deletedRoutesIds.end();
    }

    bool hasReferencesToRoutes() {
        return !isDeleted() && referencesToRoutes.size() > 0;
    }

    void putReferenceToRoutes(string &repositoryFileName, vector<int32_t>& referencesToRoutes) {
        referencesToRoutesMap.insert({repositoryFileName, referencesToRoutes});
    }

    bool compareStop(SHARED_PTR<TransportStop>& thatObj) {

        if (this == thatObj.get()) {
            return true;
        } else {
            if (!thatObj.get()) {
                return false;
            }
        }

        if (id == thatObj->id && lat == thatObj->lat && lon == thatObj->lon
        && name == thatObj->name && getNamesMap(true) == thatObj->getNamesMap(true)
        && exits.size() == thatObj->exits.size()) {
            if (exits.size() > 0) {
                for (SHARED_PTR<TransportStopExit>& exit1 : exits) {
                    if(!exit1.get()) {
                        return false;
                    }
                    bool contains = false;
                    for (SHARED_PTR<TransportStopExit>& exit2 : thatObj->exits) {
                        if (exit1 == exit2 ) {
                            contains = true;
                            if (!exit1->compareExit(exit2)) {
                                return false;
                            }
                            break;
                        }
                    }
                    if (!contains) {
                        return false;
                    }
                }
            }
        } else {
            return false;
        }
        return true;
    }
    //todo check
    void setLocation(int zoom, int32_t dx, int32_t dy) {
        x31 = dx << (31 - zoom);
        y31 = dy << (31 - zoom);
        lon = getLongitudeFromTile(TRANSPORT_STOP_ZOOM, dy);
        lat = getLatitudeFromTile(TRANSPORT_STOP_ZOOM, dx);
    }

};

struct TransportSchedule {
    vector<int32_t> tripIntervals;
    vector<int32_t> avgStopIntervals;
    vector<int32_t> avgWaitIntervals;

    bool compareSchedule (const SHARED_PTR<TransportSchedule>& thatObj) {
        return tripIntervals == thatObj->tripIntervals
        && avgStopIntervals == thatObj->avgStopIntervals
        && avgWaitIntervals == thatObj->avgWaitIntervals;
    }
};

struct Node {
    int64_t id;
    double lat;
    double lon;

    Node(double lat_, double lon_, int64_t id_) {
        id = id_;
        lat = lat_;
        lon = lon_;
    }

    bool compareNode(SHARED_PTR<Node> thatObj) {
         if (this == thatObj.get()) {
            return true;
        }
        if (id == thatObj->id
        && std::abs(lat - thatObj->lat) < 0.00001 && std::abs(lon - thatObj->lon) < 0.00001) {
            return true;
        }
    }
};

struct Way {
    int64_t id;
    vector<SHARED_PTR<Node>> nodes;
    vector<int64_t> nodeIds;
    
    Way(int64_t id_) {
        id = id_;
    }

    void addNode(SHARED_PTR<Node> n) {
        nodes.push_back(n);
    }
    SHARED_PTR<Node> getFirstNode() {
        if (nodes.size() > 0) {
            return nodes.at(0);
        }
        return NULL;
    }

    int64_t getFirstNodeId() {
        if (nodes.size() > 0) {
            return nodes.at(0)->id;
        }
        return -1;
    }

    SHARED_PTR<Node> getLastNode() {
       if (nodes.size() > 0) {
            return nodes.at(nodes.size()-1);
        }
        return NULL;
    }

    int64_t getLastNodeId() {
        if (nodes.size() > 0) {
            return nodes.at(nodes.size()-1)->id;
        }
        return -1;
    }

    void reverseNodes() {
        reverse(nodes.begin(), nodes.end());
        reverse(nodeIds.begin(), nodeIds.end());
    }

    bool compareWay(SHARED_PTR<Way> thatObj)
    {
        if (this == thatObj.get() &&
                nodeIds == thatObj->nodeIds &&
                (nodes.size() == thatObj->nodes.size()))
        {
            for (int i = 0; i < nodes.size(); i++)
            {
                if (!nodes[i]->compareNode(thatObj->nodes[i]))
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            return false;
        }
    }
};

struct TransportRoute : public MapObject {
    vector<SHARED_PTR<TransportStop>> forwardStops;
    string ref;
    string routeOperator;
    string type;
    int32_t dist;
    string color;
    vector<SHARED_PTR<Way>> forwardWays; //todo is there a Way or analogue?
    SHARED_PTR<TransportSchedule> schedule;
    const double SAME_STOP = 40;
    
    TransportRoute() {
        dist = -1;
    }

    SHARED_PTR<TransportSchedule> getOrCreateSchedule() {
        if (!schedule.get()) {
            //todo check is it correct?
            TransportSchedule* s = new TransportSchedule();
            schedule = make_shared<TransportSchedule>(&s);
        } 
        return schedule;
    }
    
    void mergeForwardWays() {
        bool changed = true;
		// combine as many ways as possible
		while (changed && forwardWays.size() != 0) {
			changed = false;
			for(int32_t k = 0; k < forwardWays.size(); ) {
				// scan to merge with the next segment
				SHARED_PTR<Way> first = forwardWays.at(k);
				double d = SAME_STOP;
				bool reverseSecond = false;
				bool reverseFirst = false;
				int32_t secondInd = -1;
				for (int32_t i = k + 1; i < forwardWays.size(); i++) {
					SHARED_PTR<Way> w = forwardWays.at(i);
					double distAttachAfter = getDistance(first->getLastNode()->lat, first->getLastNode()->lon, w->getFirstNode()->lat, w->getFirstNode()->lon);
					double distReverseAttach = getDistance(first->getLastNode()->lat, first->getLastNode()->lon, w->getLastNode()->lat, w->getLastNode()->lon);
					double distAttachAfterReverse = getDistance(first->getFirstNode()->lat, first->getFirstNode()->lon, w->getFirstNode()->lat, w->getFirstNode()->lon);
					double distReverseAttachReverse = getDistance(first->getFirstNode()->lat, first->getFirstNode()->lon, w->getLastNode()->lat, w->getLastNode()->lon);
					if (distAttachAfter < d) {
						reverseSecond = false;
						reverseFirst = false;
						d = distAttachAfter;
						secondInd = i; 
					}
					if (distReverseAttach < d) {
						reverseSecond = true;
						reverseFirst = false;
						d = distReverseAttach;
						secondInd = i;
					}
					if (distAttachAfterReverse < d) {
						reverseSecond = false;
						reverseFirst = true;
						d = distAttachAfterReverse;
						secondInd = i;
					}
					if (distReverseAttachReverse < d) {
						reverseSecond = true;
						reverseFirst = true;
						d = distReverseAttachReverse;
						secondInd = i;
					}
					if (d == 0) {
						break;
					}
				}
				if (secondInd != -1) {
                    SHARED_PTR<Way> second = nullptr;
                    if (secondInd == 0) {
                        second = *forwardWays.erase(forwardWays.begin());
                    } else {
					    second = *forwardWays.erase(forwardWays.begin() + secondInd);
                    }
					if(reverseFirst) {
						first->reverseNodes();
					}
					if(reverseSecond) {
						second->reverseNodes();
					}
					for (int i = 1; i < second->nodes.size(); i++) {
						first->addNode(second->nodes.at(i));
					}
					changed = true;
				} else {
					k++;
				}
			}
		}
		if (forwardStops.size() > 0) {
			// resort ways to stops order 
            map<SHARED_PTR<Way>, pair<int, int>> orderWays;
			for (SHARED_PTR<Way> w : forwardWays) {
				pair<int, int> pair;
                pair.first = 0;
                pair.second = 0;
				SHARED_PTR<Node> firstNode = w->getFirstNode();
				SHARED_PTR<TransportStop> st = forwardStops.at(0);
				double firstDistance = getDistance(st->lat, st->lon, firstNode->lat, firstNode->lon);
				SHARED_PTR<Node> lastNode = w->getLastNode();
				double lastDistance = getDistance(st->lat, st->lon, lastNode->lat, lastNode->lon);
				for (int i = 1; i < forwardStops.size(); i++) {
					st = forwardStops[i];
					double firstd = getDistance(st->lat, st->lon, firstNode->lat, firstNode->lon);
					double lastd = getDistance(st->lat, st->lon, lastNode->lat, lastNode->lon);
					if (firstd < firstDistance) {
                        pair.first = i;
						firstDistance = firstd;
					}
					if (lastd < lastDistance) {
                        pair.second = i;
						lastDistance = lastd;
					}
				}
                orderWays[w] = pair;
                if(pair.first > pair.second) {
					w->reverseNodes();
				}
			}
			if(orderWays.size() > 1) {
                sort(forwardWays.begin(), forwardWays.end(), [orderWays](SHARED_PTR<Way>& w1, SHARED_PTR<Way>& w2) {
                    const auto is1 = orderWays.find(w1);
                    const auto is2 = orderWays.find(w2);
                    int i1 = is1 != orderWays.end() ? min(is1->second.first, is1->second.second) : 0;
                    int i2 = is2 != orderWays.end() ? min(is2->second.first, is2->second.second) : 0;
                    return i1 < i2;
                });
			}
		}
    }

    void addWay(SHARED_PTR<Way>& w) {
        forwardWays.push_back(w);
    }

    int32_t getAvgBothDistance() {
        int32_t d = 0;
        int32_t fSsize = forwardStops.size();
        for (int i = 1; i < fSsize; i++) {
            d += getDistance(forwardStops.at(i-1)->lat, forwardStops.at(i-1)->lon, forwardStops.at(i)->lat, forwardStops.at(i)->lon);
        }
        return d;
    }

    int32_t getDist() {
        if (dist == -1) {
            dist = getAvgBothDistance();
        }
        return dist;
    }

    //ui only
    // string getAdjustedRouteRef(bool small) {
    // }

    bool compareRoute(SHARED_PTR<TransportRoute> thatObj) {
        if (this == thatObj.get()) {
            return true;
        } else {
            if (!thatObj.get()) {
                return false;
            }
        }

        if (id == thatObj->id && lat == thatObj->lat && lon == thatObj->lon 
        && name == thatObj->name && getNamesMap(true) == thatObj->getNamesMap(true)
        && ref == thatObj->ref && routeOperator == thatObj->routeOperator
        && type == thatObj->type && color == thatObj->color && dist == thatObj->dist 
        //&&((schedule == NULL && thatObj->schedule == NULL) || schedule->compareSchedule(thatObj->schedule)) 
        && forwardStops.size() == thatObj->forwardStops.size() && (forwardWays.size() == thatObj->forwardWays.size())) {
            for (int i = 0; i < forwardStops.size(); i++) {
                if (!forwardStops.at(i)->compareStop(thatObj->forwardStops.at(i))) {
                    return false;
                }
            }
            for (int i = 0; i < forwardWays.size(); i++) {
                if (!forwardWays.at(i)->compareWay(thatObj->forwardWays.at(i))) {
                    return false;
                }
            }
            return true;
        } else {
            return false;
        }
    } 
};

#endif //_OSMAND_TRANSPORT_ROUTING_OBJECTS_H
