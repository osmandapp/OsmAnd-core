#ifndef _OSMAND_TRANSPORT_ROUTE_STOPS_READER_CPP
#define _OSMAND_TRANSPORT_ROUTE_STOPS_READER_CPP

#include "binaryRead.h"
#include "transportRouteStopsReader.h"
#include "transportRoutingObjects.h"

TransportRouteStopsReader::TransportRouteStopsReader(vector<BinaryMapFile*>& files) {
	// for (auto& f: files) {
	// 	routesFileCache.insert({*f, UNORDERED(map)<int32_t, SHARED_PTR<TransportRoute>>});
	// }
}

vector<SHARED_PTR<TransportStop>> readMergedTransportStops(SearchQuery& q) {
	UNORDERED(map)<int64_t, SHARED_PTR<TransportStop>> loadedTransportStops;

	// for (auto& it : routesFilesCache) {
	// 	q.transportResults.clear();
	// 	searchTransportIndex(&q, *it->first);
	// 	vector<SHARED_PTR<TransportStop>> stops = q.transportResults;
	
	// 	mergeTransportStops(*it, loadedTransportStops, stops, localFileRoutes, routeMap[*it]);
	// 	for (SHARED_PTR<TransportStop> &stop : stops) {
	// 		int64_t stopId = stop->id;
	// 		SHARED_PTR<TransportStop> multifileStop =
	// 			loadedTransportStops.find(stopId)->second;
	// 		vector<int32_t> rrs = stop->referencesToRoutes;
	// 		if (multifileStop == stop) {
	// 			// clear up so it won't be used as it is multi file stop
	// 			stop->referencesToRoutes.clear();
	// 		} else {
	// 			// add other routes
	// 			stop->referencesToRoutes.clear();
	// 		}
	// 		if (rrs.size() > 0 && !multifileStop->isDeleted()) {
	// 			for (int32_t rr : rrs) {
	// 				const auto it = localFileRoutes.find(rr);
	// 				const auto &route = it->second;
	// 				if (it == localFileRoutes.end()) {
	// 					//						OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error,
	// 					//"Something went wrong by loading route %d for stop",
	// 					//rr);
	// 				} else if (multifileStop == stop ||
	// 						   (!multifileStop->hasRoute(route->id) &&
	// 							!multifileStop->isRouteDeleted(route->id))) {
	// 					// duplicates won't be added check!
	// 					multifileStop->addRouteId(route->id);
	// 					multifileStop->addRoute(route);
	// 				}
	// 			}
	// 		}
	// 	}
	// }

	// std::vector<SHARED_PTR<TransportStop>> stopsValues;
	// stopsValues.reserve(loadedTransportStops.size());

	// // Get all values
	// std::transform(
	// 	loadedTransportStops.begin(), loadedTransportStops.end(),
	// 	back_inserter(stopsValues),
	// 	[](std::pair<int64_t, SHARED_PTR<TransportStop>> const &pair) {
	// 		return pair.second;
	// 	});
	// loadTransportSegments(stopsValues, lst);


};


UNORDERED(map)<int32_t, SHARED_PTR<TransportRoute>> TransportRouteStopsReader::mergeTransportStops(BinaryMapFile* file,
																			UNORDERED(map) < int64_t, SHARED_PTR<TransportStop>> &loadedTransportStops,
																			vector<SHARED_PTR<TransportStop>> &stops) {
		// Iterator<TransportStop> it = stops.iterator();
		// TIntObjectHashMap<TransportRoute> routesToLoad = routesFilesCache.get(reader);
		
		// while (it.hasNext()) {
		// 	TransportStop stop = it.next();
		// 	long stopId = stop.getId();
		// 	TransportStop multifileStop = loadedTransportStops.get(stopId);
		// 	long[] routesIds = stop.getRoutesIds();
		// 	long[] delRIds = stop.getDeletedRoutesIds();
		// 	if (multifileStop == null) {
		// 		loadedTransportStops.put(stopId, stop);
		// 		multifileStop = stop;
		// 		if (!stop.isDeleted()) {
		// 			putAll(routesToLoad, stop.getReferencesToRoutes());
		// 		}
		// 	} else if (multifileStop.isDeleted()) {
		// 		// stop has nothing to load, so not needed
		// 		it.remove();
		// 	} else {
		// 		if (delRIds != null) {
		// 			for (long deletedRouteId : delRIds) {
		// 				multifileStop.addDeletedRouteId(deletedRouteId);
		// 			}
		// 		}
		// 		if (routesIds != null && routesIds.length > 0) {
		// 			int[] refs = stop.getReferencesToRoutes();
		// 			for (int i = 0; i < routesIds.length; i++) {
		// 				long routeId = routesIds[i];
		// 				if (!multifileStop.hasRoute(routeId) && !multifileStop.isRouteDeleted(routeId)) {
		// 					if(!routesToLoad.containsKey(refs[i])) {
		// 						routesToLoad.put(refs[i], null);
		// 					}
		// 				}
		// 			}
		// 		} else {
		// 			if (stop.hasReferencesToRoutes()) {
		// 				// old format
		// 				putAll(routesToLoad, stop.getReferencesToRoutes());
		// 			} else {
		// 				// stop has noting to load, so not needed
		// 				it.remove();
		// 			}
		// 		}
		// 	}
		// }
		// return routesToLoad;
}

void TransportRouteStopsReader::putAll(UNORDERED(map)<int32_t, SHARED_PTR<TransportRoute>>& routesToLoad, int referenceToRoutes[]) {
	int refsize = *(&referenceToRoutes + 1) - referenceToRoutes;
	for (int k = 0; k < refsize; k++) {
		if (routesToLoad.find(referenceToRoutes[k]) == routesToLoad.end()) {
			routesToLoad.insert({referenceToRoutes[k], nullptr});
		}
	}
}

void TransportRouteStopsReader::loadRoutes(BinaryMapFile* file, UNORDERED(map)<int32_t, SHARED_PTR<TransportRoute>>& localFileRoutes) {
	if (localFileRoutes.size() > 0) {
		vector<int32_t> routesToLoad (localFileRoutes.size());
		for (auto const& kv : localFileRoutes) {
			if (kv.second == nullptr) {
				routesToLoad.push_back(kv.first);
			}
		}
		// sort(routesToLoad.begin(), routesToLoad.end());
		// vector<int32_t> referencesToLoad;
		// vector<int32_t>::iterator itr = routesToLoad.begin();
		// int32_t p = routesToLoad.at(0) + 1;
		// while (itr != routesToLoad.end()) {
		// 	int nxt = *itr;
		// 	if (p != nxt) {
		// 		if (loadedRoutes.find(nxt) != loadedRoutes.end()) {
		// 			localFileRoutes.insert(
		// 				std::pair<int64_t, SHARED_PTR<TransportRoute>>(
		// 					nxt, loadedRoutes[nxt]));
		// 		} else {
		// 			referencesToLoad.push_back(nxt);
		// 		}
		// 	}
		// 	itr++;
		// }

		// loadTransportRoutes(file, referencesToLoad, localFileRoutes);
	}
}

SHARED_PTR<TransportRoute> TransportRouteStopsReader::getCombinedRoute(SHARED_PTR<TransportRoute> route) {
	/** java
	 * 	if (!route.isIncomplete()) {
			return route;
		}
		TransportRoute c = combinedRoutesCache.get(route.getId());
		if (c == null) {
			c = combineRoute(route);
			combinedRoutesCache.put(route.getId(), c);
		}
		return c;
	 */
}
	
SHARED_PTR<TransportRoute> TransportRouteStopsReader::combineRoute(SHARED_PTR<TransportRoute> route) {
	// 1. Get all available route parts;
	auto& incompleteRoutes = findIncompleteRouteParts(route);
	if (incompleteRoutes.empty()) {
		return route;
	}
	// here could be multiple overlays between same points
	// It's better to remove them especially identical segments
	auto& allWays = getAllWays(incompleteRoutes);

	// 2. Get array of segments (each array size > 1):
	auto& stopSegments = parseRoutePartsToSegments(incompleteRoutes);

	// 3. Merge segments and remove excess missingStops (when they are closer then MISSING_STOP_SEARCH_RADIUS):
	// + Check for missingStops. If they present in the middle/there more then one segment - we have a hole in the
	// map data
	auto& mergedSegments = combineSegmentsOfSameRoute(stopSegments);

	// 4. Now we need to properly sort segments, proper sorting is minimizing distance between stops
	// So it is salesman problem, we have this solution at TspAnt, but if we know last or first segment we can solve
	// it straightforward
	vector<SHARED_PTR<TransportRoute>> firstSegment;
	vector<SHARED_PTR<TransportRoute>> lastSegment;
	for (const auto& l : mergedSegments) {
		if (!l.front()->isMissingStop()) {
			firstSegment = l;
		}
		if (!l.back()->isMissingStop()) {
			lastSegment = l;
		}
	}
	vector<vector<SHARED_PTR<TransportStop>>> sortedSegments;
	if (!firstSegment.empty()) {
		sortedSegments.push_back(firstSegment);
		const auto itFirstSegment = mergedSegments.find(firstSegment);
		if (itFirstSegment != mergedSegments.end()) {
			mergedSegments.erase(itFirstSegment);
		}
		while (!mergedSegments.empty()) {
			auto last = sortedSegments.back().back();
			auto& add = findAndDeleteMinDistance(last->lat, last->lon, mergedSegments, true);
			sortedSegments.push_back(add);
		}

	} else if (!lastSegment.empty()) {
		sortedSegments.push_back(lastSegment);
		const auto itLastSegment = mergedSegments.find(lastSegment);
		if (itLastSegment != mergedSegments.end()) {
			mergedSegments.erase(itLastSegment);
		}
		while (!mergedSegments.empty()) {
			auto first = sortedSegments.front().front();
			auto& add = findAndDeleteMinDistance(first->lat, first->lon, mergedSegments, false);
			sortedSegments.insert(sortedSegments.begin(), add);
		}
	} else {
		sortedSegments = mergedSegments;
	}
	vector<SHARED_PTR<TransportRoute>> finalList;
	for (auto& s : sortedSegments) {
		finalList.insert(finalList.end(), s.begin(), s.end());
	}
	// 5. Create combined TransportRoute and return it
	return SHARED_PTR<TransportRoute>(new TransportRoute(route, finalList, allWays));
}

vector<SHARED_PTR<TransportStop>> TransportRouteStopsReader::findAndDeleteMinDistance(double lat, double lon, 
															vector<vector<SHARED_PTR<TransportStop>>>& mergedSegments, 
															bool attachToBegin) {
	int ind = attachToBegin ? 0 : mergedSegments.front().size() - 1;
	auto stop = mergedSegments.front().at(ind);
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
	auto& res = mergedSegments.at(minInd);
	mergedSegments.erase(mergedSegments.begin() + minInd);
	return res;
}

vector<SHARED_PTR<Way>> TransportRouteStopsReader::getAllWays(vector<SHARED_PTR<TransportRoute>>& parts) {
	vector<SHARED_PTR<Way>> w;
	for (auto t : parts) {
		if (!t.forwardWays.empty()) {
			w.insert(w.end(), t.forwardWays.begin(), t.forwardWays.end());
		}
	}
	return w;
}

vector<vector<SHARED_PTR<TransportStop>>> TransportRouteStopsReader::combineSegmentsOfSameRoute(vector<vector<SHARED_PTR<TransportStop>>>& segments) { 
	vector<vector<SHARED_PTR<TransportStop>>> resultSegments;
	auto& tempResultSegments = mergeSegments(segments, resultSegments, false);
	resultSegments.clear();
	return mergeSegments(tempResultSegments, resultSegments, true);
}

vector<vector<SHARED_PTR<TransportStop>>> TransportRouteStopsReader::mergeSegments(vector<SHARED_PTR<TransportStop>>& segments, 
																					vector<SHARED_PTR<TransportStop>>& resultSegments, 
																					bool mergeMissingStops) {
	/** java
	 * while (!segments.isEmpty()) {
			List<TransportStop> firstSegment = segments.poll();
			boolean merged = true;
			while (merged) {
				merged = false;
				Iterator<List<TransportStop>> it = segments.iterator();
				while (it.hasNext()) {
					List<TransportStop> segmentToMerge = it.next();
					if (mergeMissingSegs) {
						merged = tryToMergeMissingStops(firstSegment, segmentToMerge);
					} else {						
						merged = tryToMerge(firstSegment, segmentToMerge);
					}

					if (merged) {
						it.remove();
						break;
					}
				}
			}
			resultSegments.add(firstSegment);
		}
		return resultSegments;
	 */ 
}
	
bool TransportRouteStopsReader::tryToMerge(vector<SHARED_PTR<TransportStop>>& firstSegment, vector<SHARED_PTR<TransportStop>>& segmentToMerge) {
	/** java:
	 * 		if (firstSegment.size() < 2 || segmentToMerge.size() < 2) {
			return false;
		}
		// 1st we check that segments overlap by stop
		int commonStopFirst = 0;
		int commonStopSecond = 0;
		boolean found = false;
		for (; commonStopFirst < firstSegment.size(); commonStopFirst++) {
			for (commonStopSecond = 0; commonStopSecond < segmentToMerge.size() && !found; commonStopSecond++) {
				long lid1 = firstSegment.get(commonStopFirst).getId();
				long lid2 = segmentToMerge.get(commonStopSecond).getId();
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
					|| (leftPartFirst == leftPartSecond && firstSegment.get(firstSegment.size() - 1).isMissingStop())) {
				while (firstSegment.size() > commonStopFirst) {
					firstSegment.remove(firstSegment.size() - 1);
				}
				for (int i = commonStopSecond; i < segmentToMerge.size(); i++) {
					firstSegment.add(segmentToMerge.get(i));
				}
			}
			// merge first part
			if (commonStopFirst < commonStopSecond
					|| (commonStopFirst == commonStopSecond && firstSegment.get(0).isMissingStop())) {
				for (int i = 0; i <= commonStopFirst; i++) {
					firstSegment.remove(0);
				}
				for (int i = commonStopSecond; i >= 0; i--) {
					firstSegment.add(0, segmentToMerge.get(i));
				}
			}
			return true;

		}

		return false;
	}
	 */
}

bool TransportRouteStopsReader::tryToMergeMissingStops(vector<SHARED_PTR<TransportStop>>& firstSegment, vector<SHARED_PTR<TransportStop>>& segmentToMerge) {
	/** java:
	 // no common stops, so try to connect to the end or beginning
		// beginning
		boolean merged = false;
		if (MapUtils.getDistance(firstSegment.get(0).getLocation(),
				segmentToMerge.get(segmentToMerge.size() - 1).getLocation()) < MISSING_STOP_SEARCH_RADIUS 
				&& firstSegment.get(0).isMissingStop() && segmentToMerge.get(segmentToMerge.size() - 1).isMissingStop()) {
			firstSegment.remove(0);
			for (int i = segmentToMerge.size() - 2; i >= 0; i--) {
				firstSegment.add(0, segmentToMerge.get(i));
			}
			merged = true;
		} else if (MapUtils.getDistance(firstSegment.get(firstSegment.size() - 1).getLocation(),
				segmentToMerge.get(0).getLocation()) < MISSING_STOP_SEARCH_RADIUS
				&& segmentToMerge.get(0).isMissingStop() && firstSegment.get(firstSegment.size() - 1).isMissingStop()) {
			firstSegment.remove(firstSegment.size() - 1);
			for (int i = 1; i < segmentToMerge.size(); i++) {
				firstSegment.add(segmentToMerge.get(i));
			}
			merged = true;
		}
		return merged;
	 */
}

vector<vector<SHARED_PTR<TransportStop>>> TransportRouteStopsReader::parseRoutePartsToSegments(vector<SHARED_PTR<TransportRoute>> routeParts) {
	/** java:
		// here we assume that missing stops come in pairs <A, B, C, MISSING, MISSING, D, E...>
		// we don't add segments with 1 stop cause they are irrelevant further
		for (TransportRoute part : routeParts) {
			List<TransportStop> newSeg = new ArrayList<TransportStop>();
			for (TransportStop s : part.getForwardStops()) {
				newSeg.add(s);
				if (s.isMissingStop()) {
					if (newSeg.size() > 1) {
						segs.add(newSeg);
						newSeg = new ArrayList<TransportStop>();
					}
				}
			}
			if (newSeg.size() > 1) {
				segs.add(newSeg);
			}
		}
		return segs;
	 * 
	 */
}

vector<SHARED_PTR<TransportRoute>> TransportRouteStopsReader::findIncompleteRouteParts(SHARED_PTR<TransportRoute>& baseRoute) {
	/** java:
	 * List<TransportRoute> allRoutes = null;
		for (BinaryMapIndexReader bmir : routesFilesCache.keySet()) {
			// here we could limit routeMap indexes by only certain bbox around start / end (check comment on field)
			IncompleteTransportRoute ptr = bmir.getIncompleteTransportRoutes().get(baseRoute.getId());
			if (ptr != null) {
				TIntArrayList lst = new TIntArrayList();
				while (ptr != null) {
					lst.add(ptr.getRouteOffset());
					ptr = ptr.getNextLinkedRoute();
				}
				if (lst.size() > 0) {
					if (allRoutes == null) {
						allRoutes = new ArrayList<TransportRoute>();
					}
					allRoutes.addAll(bmir.getTransportRoutes(lst.toArray()).valueCollection());
				}
			}
		}
		return allRoutes;
	 * 
	 */ 
}

#endif
