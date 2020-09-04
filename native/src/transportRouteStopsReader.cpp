#ifndef _OSMAND_TRANSPORT_ROUTE_STOPS_READER_CPP
#define _OSMAND_TRANSPORT_ROUTE_STOPS_READER_CPP

#include "binaryRead.h"
#include "transportRouteStopsReader.h"
#include "transportRoutingObjects.h"
#include <algorithm>
#include <iterator>
#include "Logging.h"

TransportRouteStopsReader::TransportRouteStopsReader(vector<BinaryMapFile*>& files) {
	for (auto& f: files) {
		if (f->transportIndexes.size() > 0) {
			routesFilesCache.insert(make_pair(f, PT_ROUTE_MAP()));
		}
	}
}

vector<SHARED_PTR<TransportStop>> TransportRouteStopsReader::readMergedTransportStops(SearchQuery* q) {
	UNORDERED(map)<int64_t, SHARED_PTR<TransportStop>> loadedTransportStops;

	for (auto& it : routesFilesCache) {
		q->transportResults.clear();
		searchTransportIndex(q, it.first);
		vector<SHARED_PTR<TransportStop>> stops = q->transportResults;
		auto& routesToLoad = routesFilesCache[it.first];
		mergeTransportStops(routesToLoad, loadedTransportStops, stops);
		loadRoutes(it.first, routesToLoad);

		for (SHARED_PTR<TransportStop> &stop : stops) {
			if (stop->isMissingStop()) {
				continue;
			}
			int64_t stopId = stop->id;
			SHARED_PTR<TransportStop> multifileStop = loadedTransportStops.find(stopId)->second;
			vector<int32_t> rrs = stop->referencesToRoutes;
			// clear up so it won't be used as it is multi file stop
			stop->referencesToRoutes.clear();
			
			if (rrs.size() > 0 && !multifileStop->isDeleted()) {
				for (int32_t rr : rrs) {
					if (routesToLoad.find(rr) == routesToLoad.end()) {
					//skip, we already has this route from other map?
					} else if (routesToLoad.at(rr) == nullptr) {
						OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error,
						"Something went wrong by loading route %d for stop %d", rr, stop->id);
					} else {
						const auto it = routesToLoad.find(rr);
						auto &route = it->second;	
						SHARED_PTR<TransportRoute> combinedRoute = getCombinedRoute(route); 
						if (multifileStop == stop ||
							   (!multifileStop->hasRoute(combinedRoute->id) &&
								!multifileStop->isRouteDeleted(combinedRoute->id))) {
							// duplicates won't be added check!
							if(std::find(multifileStop->routesIds.begin(), multifileStop->routesIds.end(), combinedRoute->id) != multifileStop->routesIds.end()) {
								multifileStop->addRouteId(combinedRoute->id);
							}
							multifileStop->addRoute(combinedRoute);
						}
					}
				}
			}
		}
	}

	std::vector<SHARED_PTR<TransportStop>> stopsValues;
	stopsValues.reserve(loadedTransportStops.size());

	// // Get all values
	std::transform(
		loadedTransportStops.begin(), loadedTransportStops.end(),
		back_inserter(stopsValues),
		[](std::pair<int64_t, SHARED_PTR<TransportStop>> const &pair) {
			return pair.second;
	});
	return stopsValues;
}


void TransportRouteStopsReader::mergeTransportStops(PT_ROUTE_MAP &routesToLoad,
													UNORDERED(map) <int64_t, SHARED_PTR<TransportStop>> &loadedTransportStops,
													vector<SHARED_PTR<TransportStop>> &stops) {
	
	vector<SHARED_PTR<TransportStop>>::iterator it = stops.begin();
	
	while (it != stops.end()) {
		const auto stop = *it;
		int64_t stopId = stop->id;
		const auto multiStopIt = loadedTransportStops.find(stopId);
		SHARED_PTR<TransportStop> multifileStop =
			multiStopIt == loadedTransportStops.end() ? nullptr : multiStopIt->second;
		vector<int64_t>& routesIds = stop->routesIds;
		vector<int64_t>& delRIds = stop->deletedRoutesIds;
		if (multifileStop == nullptr) {
			loadedTransportStops.insert({stopId, stop});
			multifileStop = *it;
			if (!stop->isDeleted()) {
				putAll(routesToLoad, (*it)->referencesToRoutes);
			}
		} else if (multifileStop->isDeleted()) {
			it = stops.erase(it);
			continue;
		} else {
			if (delRIds.size() > 0) {
				for (vector<int64_t>::iterator it = delRIds.begin();
					it != delRIds.end(); it++) {
					multifileStop->deletedRoutesIds.push_back(*it);
				}
			}
			if (routesIds.size() > 0) {
				vector<int32_t> refs = stop->referencesToRoutes;
				for (int32_t i = 0; i < routesIds.size(); i++) {
					int64_t routeId = routesIds.at(i);
					if (find(routesIds.begin(), routesIds.end(), routeId) ==
							multifileStop->routesIds.end() && !multifileStop->isRouteDeleted(routeId)) {
						if (routesToLoad.find(refs[i]) == routesToLoad.end()) {
							routesToLoad.insert( {refs[i], nullptr});
						}
					}
				}
			} else {
				if (stop->referencesToRoutes.size() > 0) {
					putAll(routesToLoad, stop->referencesToRoutes);
				} else {
					it = stops.erase(it);
					continue;
				}
			}
		}
		it++;
	}
}

void TransportRouteStopsReader::putAll(PT_ROUTE_MAP& routesToLoad, vector<int32_t> referenceToRoutes) {
	for (int k = 0; k < referenceToRoutes.size(); k++) {
		if (routesToLoad.find(referenceToRoutes[k]) == routesToLoad.end()) {
			routesToLoad.insert({referenceToRoutes[k], nullptr});
		}
	}
}

void TransportRouteStopsReader::loadRoutes(BinaryMapFile* file, PT_ROUTE_MAP& localFileRoutes) {
	if (localFileRoutes.size() > 0) {
		vector<int32_t> routesToLoad;
		for (auto& itr : localFileRoutes) {
			if (itr.second == nullptr) {
				routesToLoad.push_back(itr.first);
			}
		}
		sort(routesToLoad.begin(), routesToLoad.end());
		loadTransportRoutes(file, routesToLoad, localFileRoutes);
	}
}

SHARED_PTR<TransportRoute> TransportRouteStopsReader::getCombinedRoute(SHARED_PTR<TransportRoute>& route) {
	if (!route->isIncomplete()) {
		return route;
	}
	shared_ptr<TransportRoute> c = combinedRoutesCache.find(route->id) != combinedRoutesCache.end() 
		? combinedRoutesCache[route->id] : nullptr;
	if (c == nullptr) {
		c = combineRoute(route);
		combinedRoutesCache.insert({route->id, c});
	}
	return c;
}
	
SHARED_PTR<TransportRoute> TransportRouteStopsReader::combineRoute(SHARED_PTR<TransportRoute>& route) {
	// 1. Get all available route parts;
	vector<SHARED_PTR<TransportRoute>> incompleteRoutes = findIncompleteRouteParts(route);
	if (incompleteRoutes.empty()) {
		return route;
	}
	// here could be multiple overlays between same points
	// It's better to remove them especially identical segments
	vector<SHARED_PTR<Way>> allWays = getAllWays(incompleteRoutes);

	// 2. Get array of segments (each array size > 1):
	vector<PT_STOPS_SEGMENT> stopSegments = parseRoutePartsToSegments(incompleteRoutes);

	// 3. Merge segments and remove excess missingStops (when they are closer then MISSING_STOP_SEARCH_RADIUS):
	// + Check for missingStops. If they present in the middle/there more then one segment - we have a hole in the
	// map data
	vector<PT_STOPS_SEGMENT> mergedSegments = combineSegmentsOfSameRoute(stopSegments);

	// 4. Now we need to properly sort segments, proper sorting is minimizing distance between stops
	// So it is salesman problem, we have this solution at TspAnt, but if we know last or first segment we can solve
	// it straightforward
	PT_STOPS_SEGMENT firstSegment;
	PT_STOPS_SEGMENT lastSegment;
	for (const auto& l : mergedSegments) {
		if (!l.front()->isMissingStop()) {
			firstSegment = l;
		}
		if (!l.back()->isMissingStop()) {
			lastSegment = l;
		}
	}
	vector<PT_STOPS_SEGMENT> sortedSegments;
	if (!firstSegment.empty()) {
		sortedSegments.push_back(firstSegment);
		const auto it = std::find(mergedSegments.begin(), mergedSegments.end(), firstSegment);
		if (it != mergedSegments.end()) {
			mergedSegments.erase(it);
		}
		while (!mergedSegments.empty()) {
			auto last = sortedSegments.back().back();
			auto add = findAndDeleteMinDistance(last->lat, last->lon, mergedSegments, true);
			sortedSegments.push_back(add);
		}

	} else if (!lastSegment.empty()) {
		sortedSegments.push_back(lastSegment);
		const auto it = std::find(mergedSegments.begin(), mergedSegments.end(), lastSegment);
		if (it != mergedSegments.end()) {
			mergedSegments.erase(it);
		}
		while (!mergedSegments.empty()) {
			auto first = sortedSegments.front().front();
			auto add = findAndDeleteMinDistance(first->lat, first->lon, mergedSegments, false);
			sortedSegments.insert(sortedSegments.begin(), add);
		}
	} else {
		sortedSegments = mergedSegments;
	}
	vector<SHARED_PTR<TransportStop>> finalList;
	for (auto& s : sortedSegments) {
		finalList.insert(finalList.end(), s.begin(), s.end());
	}
	// 5. Create combined TransportRoute and return it
	return make_shared<TransportRoute>(route, finalList, allWays);
}

PT_STOPS_SEGMENT TransportRouteStopsReader::findAndDeleteMinDistance(double lat, double lon, 
															vector<PT_STOPS_SEGMENT>& mergedSegments, 
															bool attachToBegin) {
	int ind = attachToBegin ? 0 : mergedSegments.front().size() - 1;
	auto stop = mergedSegments[0][ind];
	double minDist = getDistance(stop->lat, stop->lon, lat, lon);
	int minInd = 0;
	for (int i = 1; i < mergedSegments.size(); i++) {
		ind = attachToBegin ? 0 : mergedSegments.at(i).size() - 1;
		auto s = mergedSegments.at(i).at(ind);
		double dist = getDistance(s->lat, s->lon, lat, lon);
		if (dist < minDist) {
			minInd = i;
		}
	}
	auto res = mergedSegments.at(minInd);
	mergedSegments.erase(mergedSegments.begin() + minInd);
	return res;
}

vector<SHARED_PTR<Way>> TransportRouteStopsReader::getAllWays(vector<SHARED_PTR<TransportRoute>>& parts) {
	vector<SHARED_PTR<Way>> w;
	for (auto t : parts) {
		if (!t->forwardWays.empty()) {
			w.insert(w.end(), t->forwardWays.begin(), t->forwardWays.end());
		}
	}
	return w;
}

vector<PT_STOPS_SEGMENT> TransportRouteStopsReader::combineSegmentsOfSameRoute(vector<PT_STOPS_SEGMENT>& segs) { 
	vector<PT_STOPS_SEGMENT> resultSegments;
	auto tempResultSegments = mergeSegments(segs, resultSegments, false);
	resultSegments.clear();
	return mergeSegments(tempResultSegments, resultSegments, true);
}

vector<PT_STOPS_SEGMENT> TransportRouteStopsReader::mergeSegments(vector<PT_STOPS_SEGMENT>& segments,
																  vector<PT_STOPS_SEGMENT>& resultSegments,
																  bool mergeMissingSegs) {
	std::deque<PT_STOPS_SEGMENT> segQueue(segments.begin(), segments.end()); //check
	while (!segQueue.empty()) {
		auto firstSegment = segQueue.front();
		segQueue.pop_front();
		bool merged = true;
		while (merged) {
			merged = false;
			deque<PT_STOPS_SEGMENT>::iterator it = segQueue.begin();
			while(it != segQueue.end()) {
				auto segmentToMerge = *it;
				if (mergeMissingSegs) {
					merged = tryToMergeMissingStops(firstSegment, segmentToMerge);	
				} else {
					merged = tryToMerge(firstSegment, segmentToMerge);
				}
				if (merged) {
					it = segQueue.erase(it);
				}
				if (it != segQueue.end()) {
					it++;
				}
			}
		}
		resultSegments.push_back(firstSegment);
	}
	return resultSegments;
}

bool TransportRouteStopsReader::tryToMerge(PT_STOPS_SEGMENT& firstSegment, PT_STOPS_SEGMENT& segmentToMerge) {
	if (firstSegment.size() < 2 || segmentToMerge.size() < 2) {
		return false;
	}
	int commonStopFirst = 0;
	int commonStopSecond = 0;
	bool found = false;
	for (; commonStopFirst < firstSegment.size(); commonStopFirst++) {
		for (commonStopSecond = 0;  commonStopSecond < segmentToMerge.size() && !found; commonStopSecond++) {
			auto lid1 = firstSegment[commonStopFirst]->id;
			auto lid2 = segmentToMerge[commonStopSecond]->id;
			if (lid1 > 0 && lid2 == lid1) {
				found = true;
				break;
			}
		}
		if (found) {
			// important to increment break inside loop
			break;
		}
	}
	if (found && commonStopFirst < firstSegment.size()) {
		// we've found common stop so we can merge based on stops
		// merge last part first
		int leftPartFirst = firstSegment.size() - commonStopFirst;
		int leftPartSecond = segmentToMerge.size() - commonStopSecond;
		if (leftPartFirst < leftPartSecond 
				|| (leftPartFirst == leftPartSecond && firstSegment[firstSegment.size()-1]->isMissingStop())) {
			while (firstSegment.size() > commonStopFirst) {
				const auto it = firstSegment.end()-1;
				firstSegment.erase(it);
				// firstSegment.erase(firstSegment.size()-1);
			}
			for (int i = commonStopSecond; i < segmentToMerge.size(); i++) {
				firstSegment.push_back(segmentToMerge[i]);
			}
		}
		// merge first part
		if (commonStopFirst < commonStopSecond 
				|| (commonStopFirst == commonStopSecond && firstSegment[0]->isMissingStop())) {
			for (int i = 0; i <= commonStopFirst; i++) {
				const auto it = firstSegment.begin();
				firstSegment.erase(it);
				// firstSegment.erase(0);
			}
			for (int i = commonStopSecond; i >= 0; i--) {
				firstSegment.insert(firstSegment.begin(), segmentToMerge[i]);
			}
		}
		return true;
	}
	return false;

}

bool TransportRouteStopsReader::tryToMergeMissingStops(PT_STOPS_SEGMENT& firstSegment, PT_STOPS_SEGMENT& segmentToMerge) {
// no common stops, so try to connect to the end or beginning
	// beginning
	bool merged = false;	
	if (getDistance(firstSegment[0]->lat, firstSegment[0]->lon, segmentToMerge[segmentToMerge.size() - 1]->lat,
					segmentToMerge[segmentToMerge.size() - 1]->lon) < MISSING_STOP_SEARCH_RADIUS &&
		firstSegment[0]->isMissingStop() && segmentToMerge[segmentToMerge.size() - 1]->isMissingStop()) {
		const auto it = firstSegment.begin();
		if (it != firstSegment.end()) {
			firstSegment.erase(it);
		}
		for (int i = segmentToMerge.size()-2; i >= 0; i--) {
			firstSegment.insert(firstSegment.begin(), segmentToMerge[i]);
		}
		merged = true;
	} else if (getDistance(firstSegment[firstSegment.size()-1]->lat, firstSegment[firstSegment.size()-1]->lon, 
	segmentToMerge[0]->lat, segmentToMerge[0]->lon) < MISSING_STOP_SEARCH_RADIUS && segmentToMerge[0]->isMissingStop() && firstSegment[firstSegment.size()-1]->isMissingStop()) {
		const auto it = firstSegment.end()-1;
		if (it != firstSegment.end()) {
			firstSegment.erase(it);
		}
		for (int i = 1; i < segmentToMerge.size(); i++) {
			firstSegment.push_back(segmentToMerge[i]);
		}
		merged = true;
	}
	return merged;
}

vector<PT_STOPS_SEGMENT> TransportRouteStopsReader::parseRoutePartsToSegments(vector<SHARED_PTR<TransportRoute>> routeParts) {
	vector<PT_STOPS_SEGMENT> segs;
	// here we assume that missing stops come in pairs <A, B, C, MISSING, MISSING, D, E...>
	// we don't add segments with 1 stop cause they are irrelevant further
	for (auto& part : routeParts) {
		PT_STOPS_SEGMENT newSeg;
		for (auto& s : part->forwardStops) {
			newSeg.push_back(s);
			if (s->isMissingStop()) {
				if (newSeg.size() > 1) {
					segs.push_back(newSeg);
					newSeg = PT_STOPS_SEGMENT();
				}			
			}
		}
		if (newSeg.size() > 1) {
			segs.push_back(newSeg);
		}
	}
	return segs;
}

vector<SHARED_PTR<TransportRoute>> TransportRouteStopsReader::findIncompleteRouteParts(SHARED_PTR<TransportRoute>& baseRoute) {
	vector<SHARED_PTR<TransportRoute>> allRoutes;
	for (auto& f : routesFilesCache) {
		 UNORDERED(map)<int64_t, SHARED_PTR<TransportRoute>> filesRoutesMap;
		// here we could limit routeMap indexes by only certain bbox around start / end (check comment on field)
		vector<int32_t> lst;
		
		getIncompleteTransportRoutes(f.first);
		
		if (f.first->incompleteTransportRoutes.find(baseRoute->id) != f.first->incompleteTransportRoutes.end()) {
			shared_ptr<IncompleteTransportRoute> ptr  = f.first->incompleteTransportRoutes[baseRoute->id];
			while (ptr != nullptr) {
				lst.push_back(ptr->routeOffset);
				ptr = ptr->getNextLinkedRoute();
			}
		}
		if (lst.size() > 0) {
			sort(lst.begin(), lst.end());
			loadTransportRoutes(f.first, lst, filesRoutesMap);
			for (auto& r : filesRoutesMap) {
				allRoutes.push_back(r.second);
			}
		}
	}
	return allRoutes;
}

#endif
