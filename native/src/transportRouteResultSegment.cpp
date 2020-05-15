#ifndef _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_CPP
#define _OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_CPP
#include "transportRouteResultSegment.h"

#include "transportRoutingObjects.h"

TransportRouteResultSegment::TransportRouteResultSegment() {}

int TransportRouteResultSegment::getArrivalTime() {
	if (depTime != -1) {
		int32_t tm = depTime;
		std::vector<int32_t> intervals = route->schedule.avgStopIntervals;
		for (int i = start; i <= end; i++) {
			if (i == end) {
				return tm;
			}
			if (intervals.size() > 1) {
				tm += intervals.at(i);
			} else {
				break;
			}
		}
	}
	return -1;
}

double TransportRouteResultSegment::getTravelDist() {
	double d = 0;
	for (int32_t k = start; k < end; k++) {
		const auto &stop = route->forwardStops[k];
		const auto &nextStop = route->forwardStops[k + 1];
		d += getDistance(stop->lat, stop->lon, nextStop->lat, nextStop->lon);
	}
	return d;
}

void TransportRouteResultSegment::getGeometry(vector<Way>& list) {
	route->mergeForwardWays();
	if (DISPLAY_FULL_SEGMENT_ROUTE) {
		if (route->forwardWays.size() > DISPLAY_SEGMENT_IND) {
			list.push_back(route->forwardWays[DISPLAY_SEGMENT_IND]);
			return;
		}
		list.insert(list.end(), route->forwardWays.begin(), route->forwardWays.end());
		return;
	}
	vector<Way> fw = route->forwardWays;
	double minStart = 150;
	double minEnd = 150;

	double startLat = getStart().lat;
	double startLon = getStart().lon;
	double endLat = getEnd().lat;
	double endLon = getEnd().lon;

	int endInd = -1;
	vector<Node> res;
	for (auto it = fw.begin(); it != fw.end(); ++it) {
		vector<Node> nodes = (*it).nodes;
		for (auto nodesIt = nodes.begin(); nodesIt != nodes.end(); ++nodesIt) {
			const auto n = *nodesIt;
			double startDist = getDistance(startLat, startLon, n.lat, n.lon);
			if (startDist < minStart) {
				minStart = startDist;
				res.clear();
			}
			res.push_back(n);
			double endDist = getDistance(endLat, endLon, n.lat, n.lon);
			if (endDist < minEnd) {
				endInd = res.size();
				minEnd = endDist;
			}
		}
	}
	Way way;
	if (res.size() == 0 || endInd == -1) {
		way = Way(STOPS_WAY_ID);
		for (int i = start; i <= end; i++) {
			double lLat = getStop(i).lat;
			double lLon = getStop(i).lon;
			Node n(lLat, lLon);
			way.addNode(n);
		}
	} else {
		way = Way(GEOMETRY_WAY_ID);
		for (int k = 0; k < res.size() && k < endInd; k++) {
			way.addNode(res[k]);
		}
	}
	list.push_back(way);
}

const TransportStop& TransportRouteResultSegment::getStart() {
	return *route->forwardStops.at(start).get();
}

const TransportStop& TransportRouteResultSegment::getEnd() {
	return *route->forwardStops.at(end).get();
}

vector<SHARED_PTR<TransportStop>>
TransportRouteResultSegment::getTravelStops() {
	return vector<SHARED_PTR<TransportStop>>(
		route->forwardStops.begin() + start, route->forwardStops.begin() + end + 1);
}

const TransportStop& TransportRouteResultSegment::getStop(int32_t i) {
	return *route->forwardStops.at(i).get();
}

#endif /*_OSMAND_TRANSPORT_ROUTE_RESULT_SEGMENT_CPP*/
