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

void TransportRouteResultSegment::getGeometry(vector<shared_ptr<Way>>& list) {
	route->mergeForwardWays();
	if (DISPLAY_FULL_SEGMENT_ROUTE) {
		if (route->forwardWays.size() > DISPLAY_SEGMENT_IND && DISPLAY_SEGMENT_IND != -1) {
			list.push_back(route->forwardWays[DISPLAY_SEGMENT_IND]);
			return;
		}
		list.insert(list.end(), route->forwardWays.begin(), route->forwardWays.end());
		return;
	}
	vector<shared_ptr<Way>> ways = route->forwardWays;
	double minStart = 150;
	double minEnd = 150;

	const double startLat = getStart().lat;
	const double startLon = getStart().lon;
	const double endLat = getEnd().lat;
	const double endLon = getEnd().lon;

	SearchNodeInd* startInd = new SearchNodeInd();
	SearchNodeInd* endInd = new SearchNodeInd();
	
	vector<Node> res;
	for (int i = 0; i < ways.size(); i++) {
	// for (auto it = ways.begin(); it != ways.end(); ++it) {
		vector<Node> nodes = ways[i]->nodes;
		// for (auto nodesIt = nodes.begin(); nodesIt != nodes.end(); ++nodesIt) {
		for (int j = 0; j < nodes.size(); j++) {
			const auto n = nodes[j];
			double startDist = getDistance(startLat, startLon, n.lat, n.lon);
			if (startDist < startInd->dist) {
				startInd->dist = startDist;
				startInd->ind = j;
				startInd->way = ways[i];
			}
			double endDist = getDistance(endLat, endLon, n.lat, n.lon);
			if (endDist < endInd->dist) {
				endInd->dist = endDist;
				endInd->ind = j;
				endInd->way = ways[i];
			}
		}
	}
	bool validOneWay = startInd->way != nullptr && startInd->way == endInd->way && startInd->ind <= endInd->ind;
	if (validOneWay) {
		shared_ptr<Way> way = make_shared<Way>(GEOMETRY_WAY_ID);
		for (int k = startInd->ind; k <= endInd->ind; k++) {
			way->addNode(startInd->way->nodes[k]);
		}
		list.push_back(way);
		return;
	}
	bool validContinuation = startInd->way != nullptr && endInd->way != nullptr && startInd->way != endInd->way;
	if (validContinuation) {
		Node ln = startInd->way->getLastNode();
		Node fn = endInd->way->getFirstNode();
		// HERE we need to check other ways for continuation
		if (getDistance(ln.lat, ln.lon, fn.lat, fn.lon) < MISSING_STOP_SEARCH_RADIUS) {
			validContinuation = true;
		} else {
			validContinuation = false;
		}
	}
		if (validContinuation) {
			SHARED_PTR<Way> way = make_shared<Way>(GEOMETRY_WAY_ID);
			for (int k = startInd->ind; k < startInd->way->nodes.size(); k++) {
				way->addNode(startInd->way->nodes[k]);
			}
			list.push_back(way);
			way = make_shared<Way>(GEOMETRY_WAY_ID);
			for (int k = 0; k <= endInd->ind; k++) {
				way->addNode(endInd->way->nodes[k]);
			}
		list.push_back(way);
		return;
	}

	SHARED_PTR<Way> way = make_shared<Way>(STOPS_WAY_ID);
	for (int i = start; i <= end; i++) {
		double lLat = getStop(i).lat;
		double lLon = getStop(i).lon;
		Node n(lLat, lLon);
		way->addNode(n);
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
