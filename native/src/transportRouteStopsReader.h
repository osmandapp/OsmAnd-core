#ifndef _OSMAND_TRANSPORT_ROUTE_STOPS_READER_H
#define _OSMAND_TRANSPORT_ROUTE_STOPS_READER_H

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
// #include <queue> //check

struct BinaryMapFile;
struct SearchQuery;
struct TransportStop;
struct TransportRoute;
struct Way;

const static int MISSING_STOP_SEARCH_RADIUS = 15000;
typedef UNORDERED(map)<int64_t, shared_ptr<TransportRoute>> PT_ROUTE_MAP;
typedef vector<SHARED_PTR<TransportStop>> PT_STOPS_SEGMENT;

struct TransportRouteStopsReader {
	TransportRouteStopsReader(vector<BinaryMapFile*>& files);

	PT_ROUTE_MAP combinedRoutesCache;
	UNORDERED(map)<BinaryMapFile*, PT_ROUTE_MAP> routesFilesCache;

	PT_STOPS_SEGMENT readMergedTransportStops(SearchQuery* sr);
	void mergeTransportStops(PT_ROUTE_MAP& routesToLoad,
							UNORDERED(map) <int64_t, SHARED_PTR<TransportStop>> &loadedTransportStops,
							PT_STOPS_SEGMENT &stops);

	void putAll(PT_ROUTE_MAP& routesToLoad, vector<int32_t> referenceToRoutes);
	void loadRoutes(BinaryMapFile* file, PT_ROUTE_MAP& localFileRoutes);
	SHARED_PTR<TransportRoute> getCombinedRoute(SHARED_PTR<TransportRoute>& route);
	SHARED_PTR<TransportRoute> combineRoute(SHARED_PTR<TransportRoute>& route);
	PT_STOPS_SEGMENT findAndDeleteMinDistance(double lat, double lon, 
																vector<PT_STOPS_SEGMENT>& mergedSegments, 
																bool attachToBegin);
	vector<SHARED_PTR<Way>> getAllWays(vector<SHARED_PTR<TransportRoute>>& parts);
	vector<PT_STOPS_SEGMENT> combineSegmentsOfSameRoute(vector<PT_STOPS_SEGMENT>& segments);
	vector<PT_STOPS_SEGMENT> mergeSegments(vector<PT_STOPS_SEGMENT>& segments, 
											vector<PT_STOPS_SEGMENT>& resultSegments, 
											bool mergeMissingSegs);
	bool tryToMerge(PT_STOPS_SEGMENT& firstSegment, PT_STOPS_SEGMENT& segmentToMerge);
	bool tryToMergeMissingStops(PT_STOPS_SEGMENT& firstSegment, PT_STOPS_SEGMENT& segmentToMerge);
	vector<PT_STOPS_SEGMENT> parseRoutePartsToSegments(vector<SHARED_PTR<TransportRoute>> routeParts);
	vector<SHARED_PTR<TransportRoute>> findIncompleteRouteParts(SHARED_PTR<TransportRoute>& baseRoute);

};

#endif