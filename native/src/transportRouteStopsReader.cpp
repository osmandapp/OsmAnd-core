#ifndef _OSMAND_TRANSPORT_ROUTE_STOPS_READER_CPP
#define _OSMAND_TRANSPORT_ROUTE_STOPS_READER_CPP

#include "binaryRead.h"
#include "transportRouteStopsReader.h"
#include "transportRoutingObjects.h"
#include <algorithm>

TransportRouteStopsReader::TransportRouteStopsReader(vector<BinaryMapFile*>& files) {
	for (auto& f: files) {
		routesFilesCache.insert(make_pair(f, PT_ROUTE_MAP()));
	}
}

vector<SHARED_PTR<TransportStop>> TransportRouteStopsReader::readMergedTransportStops(SearchQuery* q) {
	UNORDERED(map)<int64_t, SHARED_PTR<TransportStop>> loadedTransportStops;

	for (auto& it : routesFilesCache) {
		q->transportResults.clear();
		searchTransportIndex(q, it.first);
		vector<SHARED_PTR<TransportStop>> stops = q->transportResults;
		
		PT_ROUTE_MAP routesToLoad = mergeTransportStops(it.first, loadedTransportStops, stops);
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
					const auto it = routesToLoad.find(rr);
					const auto &route = it->second;
					if (it == routesToLoad.end()) {
						// OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error,
						// "Something went wrong by loading route %d for stop %d", rr, stop->id);
					} else {
						SHARED_PTR<TransportRoute> combinedRoute = getCombinedRoute(route); 
						if (multifileStop == stop ||
							   (!multifileStop->hasRoute(route->id) &&
								!multifileStop->isRouteDeleted(route->id))) {
							// duplicates won't be added check!
							multifileStop->addRouteId(combinedRoute->id);
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


PT_ROUTE_MAP TransportRouteStopsReader::mergeTransportStops(BinaryMapFile* file,
																			UNORDERED(map) < int64_t, SHARED_PTR<TransportStop>> &loadedTransportStops,
																			vector<SHARED_PTR<TransportStop>> &stops) {
	
	PT_ROUTE_MAP routesToLoad = routesFilesCache.at(file);
	vector<SHARED_PTR<TransportStop>>::iterator it = stops.begin();
	
	while (it != stops.end()) {
		const auto stop = *it;
		int64_t stopId = stop->id;
		routesToLoad.clear();
		const auto multiStopIt = loadedTransportStops.find(stopId);
		SHARED_PTR<TransportStop> multifileStop =
			multiStopIt == loadedTransportStops.end() ? nullptr
													  : multiStopIt->second;
		vector<int64_t> routesIds = stop->routesIds;
		vector<int64_t> delRIds = stop->deletedRoutesIds;
		if (multifileStop == nullptr) {
			loadedTransportStops.insert({stopId, stop});
			multifileStop = *it;
			if (!stop->isDeleted()) {
				putAll(routesToLoad, (*it)->referencesToRoutes);
			}
		} else if (multifileStop->isDeleted()) {
			it = stops.erase(it);
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
						if (routesToLoad.find(routeId) == routesToLoad.end()) {
							routesToLoad.insert( {refs[i], nullptr});
						}
					}
				}
			} else {
				if ((*it)->referencesToRoutes.size() > 0) {
					putAll(routesToLoad, (*it)->referencesToRoutes);
				} else {
					it = stops.erase(it);
				}
			}
		}
		it++;
	}
	return routesToLoad;
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
		vector<int32_t> routesToLoad (localFileRoutes.size());
		for (auto& itr : localFileRoutes) {
			if (itr.second == nullptr) {
				routesToLoad.push_back(itr.first);
			}
		}

		loadTransportRoutes(file, routesToLoad, localFileRoutes);
	}
}

SHARED_PTR<TransportRoute> TransportRouteStopsReader::getCombinedRoute(SHARED_PTR<TransportRoute> route) {
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
	vector<shared_ptr<TransportRoute>> allRoutes;
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
	return allRoutes;
}

#endif
