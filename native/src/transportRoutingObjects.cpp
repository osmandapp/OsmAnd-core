#ifndef _OSMAND_TRANSPORT_ROUTING_OBJECTS_CPP
#define _OSMAND_TRANSPORT_ROUTING_OBJECTS_CPP
#include "transportRoutingObjects.h"
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include <algorithm>

// MapObject:

MapObject::MapObject() {}

UNORDERED(map)<string, string> MapObject::getNamesMap(bool includeEn) {
	if (!includeEn || enName.size() == 0) {
		if (names.size() == 0) {
			return UNORDERED(map)<string, string>();
		}
		return names;
	}
	UNORDERED(map)<string, string> tmp;
	tmp.insert(names.begin(), names.end());
	names.insert({"en", enName});
	return names;
}

string MapObject::getName(string lang) {
	if (names.find(lang) != names.end()) return names[lang];
	return "";
}

// TransportStopExit:

TransportStopExit::TransportStopExit() {}

void TransportStopExit::setLocation(int zoom, int32_t dx, int32_t dy) {
	x31 = dx << (31 - zoom);
	y31 = dy << (31 - zoom);
	// setLocation(MapUtils.getLatitudeFromTile(zoom, dy),
	// MapUtils.getLongitudeFromTile(zoom, dx));
}

bool TransportStopExit::compareExit(SHARED_PTR<TransportStopExit>& thatObj) {
	return x31 == thatObj->x31 && y31 == thatObj->y31 && ref == thatObj->ref;
}

// TransportStop:

TransportStop::TransportStop() :
distance(0),
x31(-1),
y31(-1)
{
}

bool TransportStop::hasRoute(int64_t routeId) {
	return std::find(routesIds.begin(), routesIds.end(), routeId) !=
		   deletedRoutesIds.end();
}

bool TransportStop::isDeleted() {
	return referencesToRoutes.size() == 1 &&
		   referencesToRoutes[0] == DELETED_STOP;
}

bool TransportStop::isRouteDeleted(int64_t routeId) {
	return std::find(deletedRoutesIds.begin(), deletedRoutesIds.end(),
					 routeId) != deletedRoutesIds.end();
}

bool TransportStop::hasReferencesToRoutes() {
	return !isDeleted() && referencesToRoutes.size() > 0;
}

void TransportStop::putReferenceToRoutes(string& repositoryFileName,
										 vector<int32_t>& referencesToRoutes) {
	referencesToRoutesMap.insert({repositoryFileName, referencesToRoutes});
}

void TransportStop::addRouteId(int64_t routeId) {
	// make assumption that ids are sorted
	routesIds.push_back(routeId);
	sort(routesIds.begin(), routesIds.end());
}

void TransportStop::addRoute(SHARED_PTR<TransportRoute> rt) {
	routes.push_back(rt);
}

bool TransportStop::compareStop(SHARED_PTR<TransportStop>& thatObj) {
	if (this == thatObj.get()) {
		return true;
	} else {
		if (!thatObj.get()) {
			return false;
		}
	}

	if (id == thatObj->id && lat == thatObj->lat && lon == thatObj->lon &&
		name == thatObj->name &&
		getNamesMap(true) == thatObj->getNamesMap(true) &&
		exits.size() == thatObj->exits.size()) {
		if (exits.size() > 0) {
			for (SHARED_PTR<TransportStopExit>& exit1 : exits) {
				if (!exit1.get()) {
					return false;
				}
				bool contains = false;
				for (SHARED_PTR<TransportStopExit>& exit2 : thatObj->exits) {
					if (exit1 == exit2) {
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

void TransportStop::setLocation(int zoom, int32_t dx, int32_t dy) {
	x31 = dx << (31 - zoom);
	y31 = dy << (31 - zoom);
	lon = get31LongitudeX(x31);
	lat = get31LatitudeY(y31);
}

// TransportSchedule:

TransportSchedule::TransportSchedule() {}

bool TransportSchedule::compareSchedule(
	const SHARED_PTR<TransportSchedule>& thatObj) {
	return tripIntervals == thatObj->tripIntervals &&
		   avgStopIntervals == thatObj->avgStopIntervals &&
		   avgWaitIntervals == thatObj->avgWaitIntervals;
}

// Node:

Node::Node(double lat_, double lon_) {
	lat = lat_;
	lon = lon_;
}

std::size_t hash_value(Node const& n) {
	std::size_t result = 0;
	boost::hash_combine(result, n.lat);
	boost::hash_combine(result, n.lon);

	return result;
}

// Way:
Way::Way() { id = -1; }

Way::Way(int64_t id_) { id = id_; }

Way::Way(Way& w_) {
	id = w_.id;
	nodes = w_.nodes;
}

Way::Way(const Way& w_) {
	id = w_.id;
	nodes = w_.nodes;
}

void Way::addNode(Node& n) { nodes.push_back(n); }

Node Way::getFirstNode() {
	if (nodes.size() > 0) {
		return nodes.at(0);
	}
	return Node(0, 0);
}

Node Way::getLastNode() {
	if (nodes.size() > 0) {
		return nodes.at(nodes.size() - 1);
	}
	return Node(0, 0);
}

void Way::reverseNodes() {
	reverse(nodes.begin(), nodes.end());
}

// TransportRoute:

TransportRoute::TransportRoute() { dist = -1; }

TransportSchedule& TransportRoute::getOrCreateSchedule() {
	return schedule;
}

void TransportRoute::mergeForwardWays() {
	bool changed = true;
	// combine as many ways as possible
	while (changed && forwardWays.size() != 0) {
		changed = false;
		for (int32_t k = 0; k < forwardWays.size();) {
			// scan to merge with the next segment
			Way& first = forwardWays[k];
			double d = SAME_STOP;
			bool reverseSecond = false;
			bool reverseFirst = false;
			int32_t secondInd = -1;
			for (int32_t i = k + 1; i < forwardWays.size(); i++) {
				Way& w = forwardWays[i];
				double distAttachAfter = getDistance(
					first.getLastNode().lat, first.getLastNode().lon,
					w.getFirstNode().lat, w.getFirstNode().lon);
				double distReverseAttach = getDistance(
					first.getLastNode().lat, first.getLastNode().lon,
					w.getLastNode().lat, w.getLastNode().lon);
				double distAttachAfterReverse = getDistance(
					first.getFirstNode().lat, first.getFirstNode().lon,
					w.getFirstNode().lat, w.getFirstNode().lon);
				double distReverseAttachReverse = getDistance(
					first.getFirstNode().lat, first.getFirstNode().lon,
					w.getLastNode().lat, w.getLastNode().lon);
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
				Way second;
				if (secondInd == 0) {
					second = *forwardWays.erase(forwardWays.begin());
				} else {
					second =
						*forwardWays.erase(forwardWays.begin() + secondInd);
				}
				if (reverseFirst) {
					first.reverseNodes();
				}
				if (reverseSecond) {
					second.reverseNodes();
				}
				for (int i = 1; i < second.nodes.size(); i++) {
					first.addNode(second.nodes[i]);
				}
				changed = true;
			} else {
				k++;
			}
		}
	}
	if (forwardStops.size() > 0) {
		// resort ways to stops order
		UNORDERED_map<Way, pair<int, int>>
			orderWays;	// what's wrong with you, for Christ's sake?!
		for (Way& w : forwardWays) {
			pair<int, int> pair;
			pair.first = 0;
			pair.second = 0;
			Node firstNode = w.getFirstNode();
			SHARED_PTR<TransportStop> st = forwardStops[0];
			double firstDistance =
				getDistance(st->lat, st->lon, firstNode.lat, firstNode.lon);
			Node lastNode = w.getLastNode();
			double lastDistance =
				getDistance(st->lat, st->lon, lastNode.lat, lastNode.lon);
			for (int i = 1; i < forwardStops.size(); i++) {
				st = forwardStops[i];
				double firstd =
					getDistance(st->lat, st->lon, firstNode.lat, firstNode.lon);
				double lastd =
					getDistance(st->lat, st->lon, lastNode.lat, lastNode.lon);
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
			if (pair.first > pair.second) {
				w.reverseNodes();
			}
		}
		if (orderWays.size() > 1) {
			sort(forwardWays.begin(), forwardWays.end(),
				 [orderWays](Way& w1, Way& w2) {
					 const auto is1 = orderWays.find(w1);
					 const auto is2 = orderWays.find(w2);
					 int i1 = is1 != orderWays.end()
								  ? min(is1->second.first, is1->second.second)
								  : 0;	// check
					 int i2 = is2 != orderWays.end()
								  ? min(is2->second.first, is2->second.second)
								  : 0;	// check
					 return i1 < i2;
				 });
		}
	}
}

void TransportRoute::addWay(Way& w) { forwardWays.push_back(w); }

int32_t TransportRoute::getAvgBothDistance() {
	uint32_t d = 0;
	uint64_t fSsize = forwardStops.size();
	for (uint64_t i = 1; i < fSsize; i++) {
		d += getDistance(forwardStops.at(i - 1)->lat,
						 forwardStops.at(i - 1)->lon, forwardStops.at(i)->lat,
						 forwardStops.at(i)->lon);
	}
	return d;
}

int32_t TransportRoute::getDist() {
	if (dist == -1) {
		dist = getAvgBothDistance();
	}
	return dist;
}

string TransportRoute::getAdjustedRouteRef(bool small) {
	string adjustedRef = ref;
	if (adjustedRef.length() > 0) {
		uint64_t charPos = adjustedRef.find_last_of(':');
		if (charPos != string::npos) {
			adjustedRef = adjustedRef.substr(0, charPos);
		}
		int maxRefLength = small ? 5 : 8;
		if (adjustedRef.length() > maxRefLength) {
			adjustedRef = adjustedRef.substr(0, maxRefLength - 1) + "…";
		}
	}
	return adjustedRef;
}

bool TransportRoute::compareRoute(SHARED_PTR<TransportRoute>& thatObj) {
	if (this == thatObj.get()) {
		return true;
	} else {
		if (!thatObj.get()) {
			return false;
		}
	}

	if (id == thatObj->id && lat == thatObj->lat && lon == thatObj->lon &&
		name == thatObj->name &&
		getNamesMap(true) == thatObj->getNamesMap(true) &&
		ref == thatObj->ref && routeOperator == thatObj->routeOperator &&
		type == thatObj->type && color == thatObj->color &&
		dist == thatObj->dist
		//&&((schedule == NULL && thatObj->schedule == NULL) ||
		//schedule->compareSchedule(thatObj->schedule))
		&& forwardStops.size() == thatObj->forwardStops.size() &&
		(forwardWays.size() == thatObj->forwardWays.size())) {
		for (int i = 0; i < forwardStops.size(); i++) {
			if (!forwardStops.at(i)->compareStop(thatObj->forwardStops.at(i))) {
				return false;
			}
		}
		for (int i = 0; i < forwardWays.size(); i++) {
			if (!(forwardWays.at(i) == thatObj->forwardWays.at(i))) {
				return false;
			}
		}
		return true;
	} else {
		return false;
	}
}

#endif	//_OSMAND_TRANSPORT_ROUTING_OBJECTS_CPP
