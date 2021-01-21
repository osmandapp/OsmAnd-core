#ifndef _OSMAND_ROUTE_DATA_BUNDLE_H
#define _OSMAND_ROUTE_DATA_BUNDLE_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct RouteDataResources;

struct RouteDataBundle {
   public:
	SHARED_PTR<RouteDataResources> resources;
	UNORDERED_map<string, string> data;

	RouteDataBundle();
	RouteDataBundle(SHARED_PTR<RouteDataResources>& resources);
	RouteDataBundle(SHARED_PTR<RouteDataResources>& resources, SHARED_PTR<RouteDataBundle> bundle);

	void put(string key, string value);
	void putVector(string key, vector<uint32_t> value);
	void putVectors(string key, vector<vector<uint32_t>> value);
    
	int getInt(string key, int def);
	float getFloat(string key, float def);
	bool getBool(string key, bool def);
	int64_t getLong(string key, int64_t def);
	vector<uint32_t> getIntVector(string key, vector<uint32_t> def);
	vector<vector<uint32_t>> getIntIntVector(string key, vector<vector<uint32_t>> def);
	string getString(string key, string def);

   private:
	string intVectorToString(vector<uint32_t>& vec);
	string intIntVectorToString(vector<vector<uint32_t>>& vec);
	
	vector<uint32_t> stringToIntVector(string& a);
	vector<vector<uint32_t>> stringToIntIntVector(string& a);
};

#endif /*_OSMAND_ROUTE_DATA_BUNDLE_H*/
