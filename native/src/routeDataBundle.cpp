#ifndef _OSMAND_ROUTE_DATA_BUNDLE_CPP
#define _OSMAND_ROUTE_DATA_BUNDLE_CPP
#include "routeDataBundle.h"

#include "routeDataResources.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

RouteDataBundle::RouteDataBundle() {
}

RouteDataBundle::RouteDataBundle(SHARED_PTR<RouteDataResources>& resources) : resources(resources) {
}

RouteDataBundle::RouteDataBundle(SHARED_PTR<RouteDataResources>& resources, SHARED_PTR<RouteDataBundle> bundle) : resources(resources), data(bundle->data) {
}

void RouteDataBundle::put(string key, string value) {
	data[key] = value;
}

void RouteDataBundle::putVector(string key, vector<uint32_t> value) {
	put(key, intVectorToString(value));
}

void RouteDataBundle::putVectors(string key, vector<vector<uint32_t>> value) {
	put(key, intIntVectorToString(value));
}

string RouteDataBundle::getString(string key, string def) {
	if (data.count(key) == 1)
		return data[key];
	return def;
}

int RouteDataBundle::getInt(string key, int def) {
	if (data.count(key) == 1)
		return std::stoi(data[key]);
	return def;
}

float RouteDataBundle::getFloat(string key, float def) {
	if (data.count(key) == 1)
		return std::stof(data[key]);
	return def;
}

bool RouteDataBundle::getBool(string key, bool def) {
	if (data.count(key) == 1)
		return data[key] == "true" ? true : false;
	return def;
}

int64_t RouteDataBundle::getLong(string key, int64_t def) {
	if (data.count(key) == 1)
		return std::stoll(data[key]);
	return def;
}

vector<uint32_t> RouteDataBundle::getIntVector(string key, vector<uint32_t> def) {
	if (data.count(key) == 1) {
		auto str = data[key];
		return stringToIntVector(str);
	}
	return def;
}

vector<vector<uint32_t>> RouteDataBundle::getIntIntVector(string key, vector<vector<uint32_t>> def) {
	if (data.count(key) == 1) {
		auto str = data[key];
		return stringToIntIntVector(str);
	}
	return def;
}

string RouteDataBundle::intVectorToString(vector<uint32_t>& vec) {
	std::ostringstream oss;
	if (!vec.empty()) {
		if (vec.size() > 1) {
			std::copy(vec.begin(), vec.end() - 1, std::ostream_iterator<uint32_t>(oss, ","));
		}
		oss << vec.back();
	}
	return oss.str();
}

string RouteDataBundle::intIntVectorToString(vector<vector<uint32_t>>& vec) {
	string b;
	for (int i = 0; i < vec.size(); i++) {
		if (i > 0) {
			b += ";";
		}
		vector<uint32_t> arr = vec[i];
		if (arr.size() > 0) {
			b += intVectorToString(arr);
		}
	}
	return b;
}

vector<uint32_t> RouteDataBundle::stringToIntVector(string& a) {
		
	const auto items = split_string(a, ",");
	
	vector<uint32_t> res(items.size());
	for (int i = 0; i < items.size(); i++) {
		res[i] = stoi(items[i]);
	}
	return res;
}

vector<vector<uint32_t>> RouteDataBundle::stringToIntIntVector(string& a) {
	const auto items = split_string(a, ";");
	vector<vector<uint32_t>> res(items.size());
	for (int i = 0; i < items.size(); i++) {
		string item = items[i];
		if (item.empty()) {
			res[i] = {};
		} else {
			const auto subItems = split_string(item, ",");
			res[i] = vector<uint32_t>(subItems.size());
			for (int k = 0; k < subItems.size(); k++) {
				res[i][k] = stoi(subItems[k]);
			}
		}
	}
	return res;
}

#endif /*_OSMAND_ROUTE_DATA_BUNDLE_CPP*/
