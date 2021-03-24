#ifndef _OSMAND_BINARY_READ_H
#define _OSMAND_BINARY_READ_H

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <algorithm>
#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdint.h>
#include <stdio.h>

#include <fstream>
#include <map>
#include <string>

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "multipolygons.h"
#include "routeTypeRule.h"
#include "transportRoutingObjects.h"

#if defined(WIN32)
#define close _close
#endif
#include "renderRules.h"

static const uint MAP_VERSION = 2;
static const int SHIFT_ID = 6;
struct MapTreeBounds {
	uint32_t length;
	uint32_t filePointer;
	uint32_t mapDataBlock;
	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;

	MapTreeBounds() {
	}
};
struct RestrictionInfo {
	uint64_t to;
	uint64_t via;
	uint32_t type;

	RestrictionInfo() : to(0), via(0), type(0) {
	}
};

struct RoutingIndex;
struct RouteSubregion {
	uint32_t length;
	uint32_t filePointer;
	uint32_t mapDataBlock;
	uint32_t left;
	uint32_t right;
	uint32_t top;
	uint32_t bottom;
	std::vector<RouteSubregion> subregions;
	RoutingIndex* routingIndex;

	RouteSubregion(RoutingIndex* ind) : length(0), filePointer(0), mapDataBlock(0), routingIndex(ind) {
	}
};

struct MapRoot : MapTreeBounds {
	uint minZoom;
	uint maxZoom;
	std::vector<MapTreeBounds> bounds;
};

enum PART_INDEXES {
	MAP_INDEX = 1,
	POI_INDEX,
	ADDRESS_INDEX,
	TRANSPORT_INDEX,
	ROUTING_INDEX,
};

struct BinaryPartIndex {
	uint32_t length;
	int filePointer;
	PART_INDEXES type;
	std::string name;

	BinaryPartIndex(PART_INDEXES tp) : type(tp) {
	}
};

struct RoutingIndex : BinaryPartIndex {
	vector<RouteTypeRule> routeEncodingRules;
	UNORDERED_map<std::string, uint32_t> decodingRules;
	std::vector<RouteSubregion> subregions;
	std::vector<RouteSubregion> basesubregions;

	int nameTypeRule;
	int refTypeRule;
	int destinationTypeRule;
	int destinationRefTypeRule;
	int directionForward = -1;
	int directionBackward = -1;
	int directionTrafficSignalsForward = -1;
	int directionTrafficSignalsBackward = -1;
	int trafficSignals = -1;
	int stopSign = -1;
	int stopMinor = -1;
	int giveWaySign = -1;

	RoutingIndex()
		: BinaryPartIndex(ROUTING_INDEX), nameTypeRule(-1), refTypeRule(-1), destinationTypeRule(-1),
		  destinationRefTypeRule(-1) {
	}

	void initRouteEncodingRule(uint32_t id, std::string tag, std::string val);

	void completeRouteEncodingRules();

	uint32_t findOrCreateRouteType(const std::string& tag, const std::string& value);
    
	uint32_t searchRouteEncodingRule(const std::string& tag, const std::string& value);

	RouteTypeRule& quickGetEncodingRule(uint32_t id) {
		return routeEncodingRules[id];
	}
};

struct RouteDataObject {
	const static int RESTRICTION_SHIFT = 3;
	const static uint64_t RESTRICTION_MASK = 7;
	const static int HEIGHT_UNDEFINED = -80000;

	RoutingIndex* region;
	std::vector<uint32_t> types;
	std::vector<uint32_t> pointsX;
	std::vector<uint32_t> pointsY;
	std::vector<RestrictionInfo> restrictions;
	std::vector<std::vector<uint32_t>> pointTypes;
	std::vector<std::vector<uint32_t>> pointNameTypes;
	std::vector<std::vector<uint32_t>> pointNameIds;
	std::vector<std::vector<std::string>> pointNames;
	std::vector<double> heightDistanceArray;
	int64_t id;
	bool ownsRegion;

	UNORDERED(map)<int, std::string> names;
	vector<pair<uint32_t, uint32_t>> namesIds;

	RouteDataObject() : region(NULL), id(0), ownsRegion(false) {
	}

	RouteDataObject(RoutingIndex* region, bool ownsRegion = false) : region(region), id(0), ownsRegion(ownsRegion) {
	}

	RouteDataObject(SHARED_PTR<RouteDataObject>& copy) {
		region = copy->region;
		pointsX = copy->pointsX;
		pointsY = copy->pointsY;
		types = copy->types;
		names = copy->names;
		namesIds = copy->namesIds;
		restrictions = copy->restrictions;
		restrictions = copy->restrictions;
		pointTypes = copy->pointTypes;
		pointNames = copy->pointNames;
		pointNameTypes = copy->pointNameTypes;
		ownsRegion = copy->ownsRegion;
		id = copy->id;
	}

	~RouteDataObject() {
		if (ownsRegion && region != NULL) {
			delete region;
		}
	}

	inline string getName() {
		if (names.size() > 0) {
			return names.begin()->second;
		}
		return "";
	}

	inline string getName(string& lang, bool transliter) {
		if (!names.empty()) {
			if (lang.empty()) {
				return names[region->nameTypeRule];
			}
			for (auto it = names.begin(); it != names.end(); ++it) {
				int k = it->first;
				if (region->routeEncodingRules.size() > k) {
					if (("name:" + lang) == region->routeEncodingRules[k].getTag()) {
						return names[k];
					}
				}
			}
			string nmDef = names[region->nameTypeRule];
			if (transliter && !nmDef.empty()) {
				return transliterate(nmDef);
			}
			return nmDef;
		}
		return "";
	}

	inline string getName(string& lang) {
		return getName(lang, false);
	}

	inline string getRef(string& lang, bool transliter, bool direction) {
		if (!names.empty()) {
			if (lang.empty()) {
				return names[region->refTypeRule];
			}
			for (auto it = names.begin(); it != names.end(); ++it) {
				int k = it->first;
				if (region->routeEncodingRules.size() > k) {
					if (("ref:" + lang) == region->routeEncodingRules[k].getTag()) {
						return names[k];
					}
				}
			}
			string refDefault = names[region->refTypeRule];
			if (transliter && !refDefault.empty() && refDefault.length() > 0) {
				return transliterate(refDefault);
			}
			return refDefault;
		}
		return "";
	}

	inline string getDestinationRef(bool direction) {
		if (!names.empty()) {
			string refTag = (direction == true) ? "destination:ref:forward" : "destination:ref:backward";
			string refTagDefault = "destination:ref";
			string refDefault = "";

			for (auto it = names.begin(); it != names.end(); ++it) {
				int k = it->first;
				if (region->routeEncodingRules.size() > k) {
					if (refTag == region->routeEncodingRules[k].getTag()) {
						return names[k];
					}
					if (refTagDefault == region->routeEncodingRules[k].getTag()) {
						refDefault = names[k];
					}
				}
			}
			if (!refDefault.empty()) {
				return refDefault;
			}
			// return names.get(region.refTypeRule);
		}
		return "";
	}

	inline bool isExitPoint() {
		const auto ptSz = pointTypes.size();
		for (int i = 0; i < ptSz; i++) {
			const auto& point = pointTypes[i];
			const auto pSz = point.size();
			for (int j = 0; j < pSz; j++) {
				if (region->routeEncodingRules[point[j]].getValue() == "motorway_junction") {
					return true;
				}
			}
		}
		return false;
	}

	inline string getExitName() {
		const auto pnSz = pointNames.size();
		for (int i = 0; i < pnSz; i++) {
			const auto& point = pointNames[i];

			const auto pSz = point.size();
			for (int j = 0; j < pSz; j++) {
				if (pointNameTypes[i][j] == region->nameTypeRule) {
					return point[j];
				}
			}
		}
		return "";
	}

	inline string getExitRef() {
		const auto pnSz = pointNames.size();
		for (int i = 0; i < pnSz; i++) {
			const auto& point = pointNames[i];
			const auto pSz = point.size();
			for (int j = 0; j < pSz; j++) {
				if (pointNameTypes[i][j] == region->refTypeRule) {
					return point[j];
				}
			}
		}
		return "";
	}

	inline bool hasNameTagStartsWith(string tagStartsWith) {
		for (int nm = 0; nm < namesIds.size(); nm++) {
			auto rtr = region->quickGetEncodingRule(namesIds[nm].first);
			if (rtr.getTag().rfind(tagStartsWith, 0) == 0) {
				return true;
			}
		}
		return false;
	}

	void removePointType(int ind, int type) {
		if (ind < pointTypes.size()) {
			std::vector<uint32_t>& indArr = pointTypes[ind];
			std::vector<uint32_t>::iterator it = std::find(indArr.begin(), indArr.end(), type);
			if (it != indArr.end()) {
				indArr.erase(it);
			}
		}
	}

	void processConditionalTags(const tm& time);

	inline const string& transliterate(const string& s) {
		// TODO transliteration
		return s;
	}

	inline string getDestinationName(string& lang, bool translit, bool direction) {
		// Issue #3289: Treat destination:ref like a destination, not like a ref
		string destRef =
			((getDestinationRef(direction) == "") || getDestinationRef(direction) == getRef(lang, translit, direction))
				? ""
				: getDestinationRef(direction);
		string destRef1 = ("" == destRef ? "" : destRef + ", ");

		if (!names.empty()) {
			// Issue #3181: Parse destination keys in this order:
			//              destination:lang:XX:forward/backward
			//              destination:forward/backward
			//              destination:lang:XX
			//              destination

			string destinationTagLangFB = "destination:lang:XX";
			if (!lang.empty()) {
				destinationTagLangFB = (direction == true) ? "destination:lang:" + lang + ":forward"
														   : "destination:lang:" + lang + ":backward";
			}
			string destinationTagFB = (direction == true) ? "destination:forward" : "destination:backward";
			string destinationTagLang = "destination:lang:XX";
			if (!lang.empty()) {
				destinationTagLang = "destination:lang:" + lang;
			}
			string destinationTagDefault = "destination";
			string destinationDefault = "";

			for (auto it = names.begin(); it != names.end(); ++it) {
				int k = it->first;
				if (region->routeEncodingRules.size() > k) {
					if (!lang.empty() && destinationTagLangFB == region->routeEncodingRules[k].getTag()) {
						return destRef1 + (translit ? transliterate(names[k]) : names[k]);
					}
					if (destinationTagFB == region->routeEncodingRules[k].getTag()) {
						return destRef1 + (translit ? transliterate(names[k]) : names[k]);
					}
					if (!lang.empty() && destinationTagLang == region->routeEncodingRules[k].getTag()) {
						return destRef1 + (translit ? transliterate(names[k]) : names[k]);
					}
					if (destinationTagDefault == region->routeEncodingRules[k].getTag()) {
						destinationDefault = names[k];
					}
				}
			}
			if (!destinationDefault.empty()) {
				return destRef1 + (translit ? transliterate(destinationDefault) : destinationDefault);
			}
		}
		return "" == destRef ? "" : destRef;
	}

	inline int64_t getId() {
		return id;
	}

	int getSize() {
		int s = sizeof(this);
		s += pointsX.capacity() * sizeof(uint32_t);
		s += pointsY.capacity() * sizeof(uint32_t);
		s += types.capacity() * sizeof(uint32_t);
		s += restrictions.capacity() * sizeof(uint64_t);
		std::vector<std::vector<uint32_t>>::iterator t = pointTypes.begin();
		for (; t != pointTypes.end(); t++) {
			s += (*t).capacity() * sizeof(uint32_t);
		}
		t = pointNameTypes.begin();
		for (; t != pointNameTypes.end(); t++) {
			s += (*t).capacity() * sizeof(uint32_t);
		}
		std::vector<std::vector<std::string>>::iterator ts = pointNames.begin();
		for (; ts != pointNames.end(); ts++) {
			s += (*ts).capacity() * 10;
		}
		s += namesIds.capacity() * sizeof(pair<uint32_t, uint32_t>);
		s += names.size() * sizeof(pair<int, string>) * 10;
		return s;
	}

	inline int getLanes() {
		auto sz = types.size();
		for (int i = 0; i < sz; i++) {
			auto& r = region->quickGetEncodingRule(types[i]);
			int ln = r.lanes();
			if (ln > 0) {
				return ln;
			}
		}
		return -1;
	}

	inline int getRestrictionLength() {
		return restrictions.empty() ? 0 : (int)restrictions.size();
	}

	inline int getRestrictionType(int i) {
		return restrictions[i].type;
	}

	inline long getRestrictionId(int i) {
		return restrictions[i].to;
	}

	inline long getRestrictionVia(int i) {
		return restrictions[i].via;
	}

	bool tunnel();
	int getOneway();
	string getValue(const string& tag);
	string getValue(uint32_t pnt, const string& tag);

	inline int getPointsLength() {
		return (int)pointsX.size();
	}
	
	std::vector<uint32_t> getPointTypes(int ind) {
		if (ind >= pointTypes.size()) {
			return {};
		}
		return pointTypes[ind];
	}
	
	bool loop() {
		return pointsX[0] == pointsX[pointsX.size() - 1] && pointsY[0] == pointsY[pointsY.size() - 1];
	}

	void insert(int pos, int x31, int y31) {
		pointsX.insert(pointsX.begin() + pos, x31);
		pointsY.insert(pointsY.begin() + pos, y31);
		if (pointTypes.size() > pos) {
			std::vector<uint32_t> types;
			pointTypes.insert(pointTypes.begin() + pos, types);
		}
	}

	std::vector<double> calculateHeightArray();

	string getHighway();

	bool platform();

	bool roundabout();

	double simplifyDistance(int x, int y, int px, int py) {
		return abs(px - x) * 0.011 + abs(py - y) * 0.01863;
	}

	double distance(int startPoint, int endPoint) {
		if (startPoint > endPoint) {
			int k = endPoint;
			endPoint = startPoint;
			startPoint = k;
		}
		double d = 0;
		for (int k = startPoint; k < endPoint && k < getPointsLength() - 1; k++) {
			int x = pointsX[k];
			int y = pointsY[k];
			int kx = pointsX[k + 1];
			int ky = pointsY[k + 1];
			d += simplifyDistance(kx, ky, x, y);
		}
		return d;
	}

	float getMaximumSpeed(bool direction) {
		auto sz = types.size();
		float maxSpeed = 0;
		for (int i = 0; i < sz; i++) {
			auto& r = region->quickGetEncodingRule(types[i]);
			float mx = r.maxSpeed();
			if (mx > 0) {
				if (r.isForward() != 0) {
					if ((r.isForward() == 1) != direction) {
						continue;
					} else {
						// priority over default
						maxSpeed = mx;
						break;
					}
				} else {
					maxSpeed = mx;
				}
			}
		}
		return maxSpeed;
	}

	double directionRoute(int startPoint, bool plus) {
		// look at comment JAVA
		return directionRoute(startPoint, plus, 5);
	}

	bool bearingVsRouteDirection(double bearing) {
		bool direction = true;
		if (bearing >= 0) {
			double diff = alignAngleDifference(directionRoute(0, true) - bearing / 180.f * M_PI);
			direction = fabs(diff) < M_PI / 2.f;
		}
		return direction;
	}

	bool isRoadDeleted() {
		auto sz = types.size();
		for (int i = 0; i < sz; i++) {
			auto& r = region->quickGetEncodingRule(types[i]);
			if ("osmand_change" == r.getTag() && "delete" == r.getValue()) {
				return true;
			}
		}
		return false;
	}

	bool isStopApplicable(bool direction, int intId, int startPointInd, int endPointInd) {
		auto pt = pointTypes[intId];
		auto sz = pt.size();
		for (int i = 0; i < sz; i++) {
			auto& r = region->quickGetEncodingRule(pt[i]);
			// Evaluate direction tag if present
			if ("direction" == r.getTag()) {
				auto dv = r.getValue();
				if ((dv == "forward" && direction) || (dv == "backward" && !direction)) {
					return true;
				} else if ((dv == "forward" && !direction) || (dv == "backward" && direction)) {
					return false;
				}
			}
			// Tagging stop=all should be ok anyway, usually tagged on intersection node itself, so not needed here
			// if (r.getTag().equals("stop") && r.getValue().equals("all")) {
			//    return true;
			//}
		}
		// Heuristic fallback: Distance analysis for STOP with no recognized directional tagging:
		// Mask STOPs closer to the start than to the end of the routing segment if it is within 50m of start, but do
		// not mask STOPs mapped directly on start/end (likely intersection node)
		auto d2Start = distance(startPointInd, intId);
		auto d2End = distance(intId, endPointInd);
		if ((d2Start < d2End) && d2Start != 0 && d2End != 0 && d2Start < 50) {
			return false;
		}
		// No directional info detected
		return true;
	}

	// Gives route direction of EAST degrees from NORTH ]-PI, PI]
	double directionRoute(int startPoint, bool plus, float dist) {
		int x = pointsX[startPoint];
		int y = pointsY[startPoint];
		int nx = startPoint;
		int px = x;
		int py = y;
		double total = 0;
		do {
			if (plus) {
				nx++;
				if (nx >= (int)pointsX.size()) {
					break;
				}
			} else {
				nx--;
				if (nx < 0) {
					break;
				}
			}
			px = pointsX[nx];
			py = pointsY[nx];
			// translate into meters
			total += abs(px - x) * 0.011 + abs(py - y) * 0.01863;
		} while (total < dist);
		return -atan2((float)x - px, (float)y - py);
	}

	static double parseLength(string v, double def) {
		double f = 0;
		// 14'10" 14 - inches, 10 feet
		int i = findFirstNumberEndIndex(v);
		if (i > 0) {
			f += strtod_li(v.substr(0, i));
			string pref = v.substr(i, v.length());
			float add = 0;
			for (int ik = 0; ik < pref.length(); ik++) {
				if ((pref[ik] >= '0' && pref[ik] <= '9') || pref[ik] == '.' || pref[ik] == '-') {
					int first = findFirstNumberEndIndex(pref.substr(ik));
					if (first != -1) {
						add = parseLength(pref.substr(ik), 0);
						pref = pref.substr(0, ik);
					}
					break;
				}
			}
			if (pref.find("km") != string::npos) {
				f *= 1000;
			}
			if (pref.find("in") != string::npos || pref.find("\"") != string::npos) {
				f *= 0.0254;
			} else if (pref.find("\'") != string::npos || pref.find("ft") != string::npos ||
					   pref.find("feet") != string::npos) {
				// foot to meters
				f *= 0.3048;
			} else if (pref.find("cm") != string::npos) {
				f *= 0.01;
			} else if (pref.find("mile") != string::npos) {
				f *= 1609.34f;
			}
			return f + add;
		}
		return def;
	}

	//	static double parseLength(string v, double def) {
	//		// 14"10' not supported
	//		int i = findFirstNumberEndIndex(v);
	//		if (i > 0) {
	//			double f = strtod_li(v.substr(0, i));
	//			if (v.find("\"") != string::npos  || v.find("ft") != string::npos) {
	//				// foot to meters
	//				f *= 0.3048;
	//			}
	//			return f;
	//		}
	//		return def;
	//	}

	static double parseWeightInTon(string v, double def) {
		int i = findFirstNumberEndIndex(v);
		if (i > 0) {
			double f = strtod_li(v.substr(0, i));
			if (v.find("\"") != string::npos || v.find("lbs") != string::npos) {
				// lbs -> kg -> ton
				f = (f * 0.4535) / 1000.0;
			}
			return f;
		}
		return def;
	}
};

struct IndexStringTable {
	uint32_t fileOffset;
	uint32_t length;
	UNORDERED(map)<int32_t, string> stringTable;
};

struct TransportIndex : BinaryPartIndex {
	int left;
	int right;
	int top;
	int bottom;

	uint32_t stopsFileOffset;
	uint32_t stopsFileLength;
	uint32_t incompleteRoutesOffset;
	uint32_t incompleteRoutesLength;

	IndexStringTable* stringTable;

	TransportIndex() : BinaryPartIndex(TRANSPORT_INDEX), left(0), right(0), top(0), bottom(0) {
	}
};

struct MapIndex : BinaryPartIndex {
	std::vector<MapRoot> levels;

	UNORDERED(map)<int, tag_value> decodingRules;
	// DEFINE hash
	// UNORDERED(map)<tag_value, int> encodingRules;

	int nameEncodingType;
	int refEncodingType;
	int coastlineEncodingType;
	int coastlineBrokenEncodingType;
	int landEncodingType;
	int onewayAttribute;
	int onewayReverseAttribute;
	UNORDERED(set)<int> positiveLayers;
	UNORDERED(set)<int> negativeLayers;

	MapIndex() : BinaryPartIndex(MAP_INDEX) {
		nameEncodingType = refEncodingType = coastlineBrokenEncodingType = coastlineEncodingType = -1;
		landEncodingType = onewayAttribute = onewayReverseAttribute = -1;
	}

	void finishInitializingTags() {
		int free = (int)decodingRules.size() * 2 + 1;
		coastlineBrokenEncodingType = free++;
		initMapEncodingRule(0, coastlineBrokenEncodingType, "natural", "coastline_broken");
		if (landEncodingType == -1) {
			landEncodingType = free++;
			initMapEncodingRule(0, landEncodingType, "natural", "land");
		}
	}

	void initMapEncodingRule(uint32_t type, uint32_t id, std::string tag, std::string val) {
		tag_value pair = tag_value(tag, val);
		// DEFINE hash
		// encodingRules[pair] = id;
		decodingRules[id] = pair;

		if ("name" == tag) {
			nameEncodingType = id;
		} else if ("natural" == tag && "coastline" == val) {
			coastlineEncodingType = id;
		} else if ("natural" == tag && "land" == val) {
			landEncodingType = id;
		} else if ("oneway" == tag && "yes" == val) {
			onewayAttribute = id;
		} else if ("oneway" == tag && "-1" == val) {
			onewayReverseAttribute = id;
		} else if ("ref" == tag) {
			refEncodingType = id;
		} else if ("layer" == tag) {
			if (val != "" && val != "0") {
				if (val[0] == '-') {
					negativeLayers.insert(id);
				} else {
					positiveLayers.insert(id);
				}
			}
		}
	}
};

struct BinaryMapFile {
	std::string inputName;
	uint32_t version;
	uint64_t dateCreated;
	std::vector<MapIndex> mapIndexes;
	std::vector<RoutingIndex*> routingIndexes;
	std::vector<TransportIndex*> transportIndexes;
	std::vector<BinaryPartIndex*> indexes;
	UNORDERED(map)<uint64_t, shared_ptr<IncompleteTransportRoute>> incompleteTransportRoutes;
	bool incompleteLoaded = false;
	int fd;
	int routefd;
	int geocodingfd;
	bool basemap;
	bool external;
	bool roadOnly;
	bool liveMap;

	bool isBasemap() {
		return basemap;
	}

	bool isExternal() {
		return external;
	}

	bool isLiveMap() {
		return liveMap;
	}
	bool isRoadOnly() {
		return roadOnly;
	}

	~BinaryMapFile() {
		close(fd);
		close(routefd);
	}
};

struct ResultPublisher {
	std::vector<FoundMapDataObject> result;
	UNORDERED(map)<uint64_t, FoundMapDataObject> ids;

	bool publish(FoundMapDataObject r);

	shared_ptr<vector<MapDataObject*>> getObjects() {
		shared_ptr<vector<MapDataObject*>> objs(new vector<MapDataObject*>);
		objs->reserve(result.size());
		for (uint i = 0; i < result.size(); i++) {
			objs->push_back(result[i].obj);
		}
		return objs;
	}

	bool publishOnlyUnique(std::vector<FoundMapDataObject>& r) {
		for (uint i = 0; i < r.size(); i++) {
			if (!publish(r[i])) {
				delete r[i].obj;
			}
		}
		r.clear();
		return true;
	}

	bool publishAll(std::vector<FoundMapDataObject>& r) {
		for (uint i = 0; i < r.size(); i++) {
			result.push_back(r[i]);
		}
		r.clear();
		return true;
	}

	void clear() {
		result.clear();
		ids.clear();
	}

	bool isCancelled() {
		return false;
	}
	virtual ~ResultPublisher() {
		deleteObjects(result);
	}
};

struct SearchQuery {
	RenderingRuleSearchRequest* req;
	int left;
	int right;
	int top;
	int bottom;
	int oceanLeft;
	int oceanRight;
	int oceanTop;
	int oceanBottom;
	uint zoom;
	ResultPublisher* publisher;

	coordinates cacheCoordinates;
	uint ocean;
	uint oceanTiles;

	uint numberOfVisitedObjects;
	uint numberOfAcceptedObjects;
	uint numberOfReadSubtrees;
	uint numberOfAcceptedSubtrees;

	int limit;

	vector<SHARED_PTR<TransportStop>> transportResults;

	// cache information
	vector<int32_t> cacheTypes;
	vector<int64_t> cacheIdsA;
	vector<int64_t> cacheIdsB;

	SearchQuery(int l, int r, int t, int b, RenderingRuleSearchRequest* req, ResultPublisher* publisher)
		: req(req), left(l), right(r), top(t), bottom(b), publisher(publisher) {
		numberOfAcceptedObjects = numberOfVisitedObjects = 0;
		numberOfAcceptedSubtrees = numberOfReadSubtrees = 0;
		oceanTiles = 0;
		ocean = 0;
		limit = -1;
	}
	SearchQuery(int l, int r, int t, int b) : left(l), right(r), top(t), bottom(b) {
	}

	SearchQuery() {
		numberOfAcceptedObjects = numberOfVisitedObjects = 0;
		numberOfAcceptedSubtrees = numberOfReadSubtrees = 0;
		oceanTiles = 0;
		ocean = 0;
		limit = -1;
	}

	bool publish(MapDataObject* obj, MapIndex* ind, int8_t zoom) {
		return publisher->publish(FoundMapDataObject(obj, ind, zoom));
	}
};

std::vector<BinaryMapFile*> getOpenMapFiles();

void searchTransportIndex(SearchQuery* q, BinaryMapFile* file);

void loadTransportRoutes(BinaryMapFile* file, vector<int32_t> filePointers, UNORDERED(map) < int64_t,
						 SHARED_PTR<TransportRoute> > &result);

void searchRouteSubregions(SearchQuery* q, std::vector<RouteSubregion>& tempResult, bool basemap, bool geocoding);

void searchRouteDataForSubRegion(SearchQuery* q, std::vector<RouteDataObject*>& list, RouteSubregion* sub,
								 bool geocoding);

ResultPublisher* searchObjectsForRendering(SearchQuery* q, bool skipDuplicates, std::string msgNothingFound,
										   int& renderedState);

BinaryMapFile* initBinaryMapFile(std::string inputName, bool useLive, bool routingOnly);

bool initMapFilesFromCache(std::string inputName);

bool closeBinaryMapFile(std::string inputName);

void getIncompleteTransportRoutes(BinaryMapFile* file);

#endif
