#ifndef _OSMAND_ROUTE_DATA_BUNDLE_CPP
#define _OSMAND_ROUTE_DATA_BUNDLE_CPP
#include "routeDataBundle.h"

#include "routeDataResources.h"

RouteDataBundle::RouteDataBundle(SHARED_PTR<RouteDataResources>& resources) : resources(resources) {
}

void RouteDataBundle::put(string key, string value) {
	data[key] = value;
}

void RouteDataBundle::putVector(string key, vector<uint32_t> value) {
	put(key, vectorToString(value));
}

void RouteDataBundle::putVectors(string key, vector<vector<uint32_t>> value) {
	put(key, vectorArrayToString(value));
}

string RouteDataBundle::getString(string key) {
	return data[key];
}

string RouteDataBundle::vectorToString(vector<uint32_t>& vec) {
	ostringstream oss;
	if (!vec.empty()) {
		if (vec.size() > 1) {
			copy(vec.begin(), vec.end() - 1, ostream_iterator<uint32_t>(oss, ";"));
		}
		oss << vec.back();
	}
	return oss.str();
}

string RouteDataBundle::vectorArrayToString(vector<vector<uint32_t>>& vec) {
	string res;
	if (!vec.empty()) {
		for (auto it = vec.begin(); vec.size() > 1 && it != vec.end() - 1; ++it) {
			res += vectorToString(*it);
			res += ";";
		}
		res += vectorToString(vec.back());
	}
	return res;
}

#endif /*_OSMAND_ROUTE_DATA_BUNDLE_CPP*/
