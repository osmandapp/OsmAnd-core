#ifndef _OSMAND_ROUTE_DATA_BUNDLE_H
#define _OSMAND_ROUTE_DATA_BUNDLE_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct RouteDataResources;

struct RouteDataBundle {
   public:
	SHARED_PTR<RouteDataResources> resources;
	UNORDERED_map<string, string> data;

	RouteDataBundle(SHARED_PTR<RouteDataResources>& resources);

	void put(string key, string value);
	void putVector(string key, vector<uint32_t> value);
	void putVectors(string key, vector<vector<uint32_t>> value);

	string getString(string key);

   private:
	string vectorToString(vector<uint32_t>& vec);
	string vectorArrayToString(vector<vector<uint32_t>>& vec);
};

#endif /*_OSMAND_ROUTE_DATA_BUNDLE_H*/
