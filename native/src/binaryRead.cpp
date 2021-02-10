#include "binaryRead.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>

#include "Logging.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/wire_format_lite.cc"
#include "google/protobuf/wire_format_lite.h"
#include "proto/OBF.pb.h"
#include "proto/osmand_index.pb.h"
#if defined(WIN32)
#undef min
#undef max
#endif

using namespace std;
#define DO_(EXPRESSION) \
	if (!(EXPRESSION)) return false
using google::protobuf::internal::WireFormatLite;
using google::protobuf::io::CodedInputStream;
using google::protobuf::io::FileInputStream;

// using namespace google::protobuf::internal;
#define INT_MAXIMUM 0x7fffffff

static uint zoomForBaseRouteRendering = 13;
static uint detailedZoomStartForRouteSection = 13;
static uint zoomOnlyForBasemaps = 11;
static uint zoomDetailedForCoastlines = 17;
std::vector<BinaryMapFile*> openFiles;
std::vector<TransportIndex*> transportIndexesList;
OsmAnd::OBF::OsmAndStoredIndex* cache = NULL;

#ifdef MALLOC_H
#include <malloc.h>
void print_dump(const char* msg1, const char* msg2) {
	struct mallinfo info = mallinfo();
	OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "MEMORY %s %s heap - %d  alloc - %d free - %d", msg1, msg2,
					  info.usmblks, info.uordblks, info.fordblks);
}
#endif

uint32_t RoutingIndex::findOrCreateRouteType(const std::string& tag, const std::string& value) {
	uint32_t i = 0;
	for (; i < routeEncodingRules.size(); i++) {
		RouteTypeRule& rtr = routeEncodingRules[i];
		if (tag == rtr.getTag() && value == rtr.getValue()) {
			return i;
		}
	}
	RouteTypeRule rtr(tag, value);
	routeEncodingRules.push_back(rtr);
	return i;
}

uint32_t RoutingIndex::searchRouteEncodingRule(const std::string& tag, const std::string& value) {
	if (decodingRules.empty()) {
		for (uint32_t i = 1; i < routeEncodingRules.size(); i++) {
			RouteTypeRule& rt = routeEncodingRules[i];
			string ks = rt.getTag() + "#" + rt.getValue();
			decodingRules[ks] = i;
		}
	}
	string k = tag + "#" + value;
	if (decodingRules.count(k) == 1) {
		return decodingRules[k];
	}
	return -1;
}

void RoutingIndex::completeRouteEncodingRules() {
	for (uint32_t i = 0; i < routeEncodingRules.size(); i++) {
		RouteTypeRule& rtr = routeEncodingRules[i];
		if (rtr.conditional()) {
			std::string tag = rtr.getNonConditionalTag();
			for (auto& c : rtr.getConditions()) {
				if (tag != "" && c.value != "") {
					c.ruleid = findOrCreateRouteType(tag, c.value);
				}
			}
		}
	}
}

void RoutingIndex::initRouteEncodingRule(uint32_t id, std::string tag, std::string val) {
	RouteTypeRule rule(tag, val);
	while (!(routeEncodingRules.size() > id)) {
		RouteTypeRule empty(tag, val);
		routeEncodingRules.push_back(empty);
	}
	routeEncodingRules[id] = rule;

	if (tag == "name") {
		nameTypeRule = id;
	} else if (tag == "ref") {
		refTypeRule = id;
	} else if (tag == "destination" || tag == "destination:forward" || tag == "destination:backward" ||
			   startsWith(tag, "destination:lang:")) {
		destinationTypeRule = id;
	} else if (tag == "destination:ref" || tag == "destination:ref:forward" || tag == "destination:ref:backward") {
		destinationRefTypeRule = id;
	} else if (tag == "highway" && val == "traffic_signals") {
		trafficSignals = id;
	} else if (tag == "highway" && val == "stop") {
		stopSign = id;
	} else if (tag == "highway" && val == "give_way") {
		giveWaySign = id;
	} else if (tag == "traffic_signals:direction") {
		if (val == "forward") {
			directionTrafficSignalsForward = id;
		} else if (val == "backward") {
			directionTrafficSignalsBackward = id;
		}
	} else if (tag == "direction") {
		if (val == "forward") {
			directionForward = id;
		} else if (val == "backward") {
			directionBackward = id;
		}
	}
}

void RouteDataObject::processConditionalTags(const tm& time) {
	auto sz = types.size();
	for (uint32_t i = 0; i < sz; i++) {
		auto& r = region->quickGetEncodingRule(types[i]);
		if (r.conditional()) {
			uint32_t vl = r.conditionalValue(time);
			if (vl > 0) {
				auto& rtr = region->quickGetEncodingRule(vl);
				std::string nonCondTag = rtr.getTag();
				uint32_t ks = 0;
				for (; ks < sz; ks++) {
					auto& toReplace = region->quickGetEncodingRule(types[ks]);
					if (toReplace.getTag() == nonCondTag) {
						types[ks] = vl;
						break;
					}
				}
				if (ks == sz) {
					types.push_back(vl);
				}
			}
		}
	}

	for (uint32_t i = 0; i < pointTypes.size(); i++) {
		std::vector<uint32_t> ptypes = pointTypes[i];
		auto pSz = ptypes.size();
		for (uint32_t j = 0; j < pSz; j++) {
			auto& r = region->quickGetEncodingRule(ptypes[j]);
			if (r.conditional()) {
				uint32_t vl = r.conditionalValue(time);
				if (vl > 0) {
					auto& rtr = region->quickGetEncodingRule(vl);
					std::string nonCondTag = rtr.getTag();
					uint32_t ks = 0;
					for (; ks < pSz; ks++) {
						auto& toReplace = region->quickGetEncodingRule(ptypes[ks]);
						if (toReplace.getTag() == nonCondTag) {
							ptypes[ks] = vl;
							break;
						}
					}
					if (ks == pSz) {
						ptypes.push_back(vl);
					}
				}
			}
		}
		pointTypes[i] = ptypes;
	}
}

bool RouteDataObject::tunnel() {
	auto sz = types.size();
	for (int i = 0; i < sz; i++) {
		auto& r = region->quickGetEncodingRule(types[i]);
		if (r.getTag() == "tunnel" && r.getValue() == "yes") {
			return true;
		}
		if (r.getTag() == "layer" && r.getValue() == "-1") {
			return true;
		}
	}
	return false;
}

int RouteDataObject::getOneway() {
	auto sz = types.size();
	for (uint32_t i = 0; i < sz; i++) {
		auto& r = region->quickGetEncodingRule(types[i]);
		if (r.onewayDirection() != 0) {
			return r.onewayDirection();
		} else if (r.roundabout()) {
			return 1;
		}
	}
	return 0;
}

string RouteDataObject::getValue(const string& tag) {
	auto sz = types.size();
	for (uint32_t i = 0; i < sz; i++) {
		auto& r = region->routeEncodingRules[types[i]];
		if (r.getTag() == tag) {
			return r.getValue();
		}
	}
	for (auto it = names.begin(); it != names.end(); ++it) {
		uint32_t k = it->first;
		if (region->routeEncodingRules.size() > k) {
			auto& r = region->routeEncodingRules[k];
			if (r.getTag() == tag) {
				return it->second;
			}
		}
	}
	return "";
}

string RouteDataObject::getValue(uint32_t pnt, const string& tag) {
	if (pointTypes.size() > pnt) {
		auto tps = pointTypes[pnt];
		auto sz = tps.size();
		for (uint32_t i = 0; i < sz; i++) {
			auto& r = region->routeEncodingRules[tps[i]];
			if (r.getTag() == tag) {
				return r.getValue();
			}
		}
	}
	if (pointNameTypes.size() > pnt) {
		auto tps = pointNameTypes[pnt];
		auto sz = tps.size();
		for (uint32_t i = 0; i < sz; i++) {
			auto& r = region->routeEncodingRules[tps[i]];
			if (r.getTag() == tag) {
				return pointNames[pnt][i];
			}
		}
	}
	return "";
}

std::vector<double> RouteDataObject::calculateHeightArray() {
	if (heightDistanceArray.size() > 0) {
		return heightDistanceArray;
	}
	string strStart = getValue("osmand_ele_start");

	if (strStart == "") {
		return heightDistanceArray;
	}
	string strEnd = getValue("osmand_ele_end");
	int startHeight = (int)atof(strStart.c_str());
	int endHeight = startHeight;
	if (strEnd != "") {
		endHeight = (int)atof(strEnd.c_str());
	}

	heightDistanceArray.resize(2 * getPointsLength(), 0);
	double plon = 0;
	double plat = 0;
	double prevHeight = startHeight;
	for (uint k = 0; k < getPointsLength(); k++) {
		double lon = get31LongitudeX(pointsX[k]);
		double lat = get31LatitudeY(pointsY[k]);
		if (k > 0) {
			double dd = getDistance(plat, plon, lat, lon);
			double height = HEIGHT_UNDEFINED;
			if (k == getPointsLength() - 1) {
				height = endHeight;
			} else {
				string asc = getValue(k, "osmand_ele_asc");
				if (asc != "") {
					height = (prevHeight + atof(asc.c_str()));
				} else {
					string desc = getValue(k, "osmand_ele_desc");
					if (desc != "") {
						height = (prevHeight - atof(desc.c_str()));
					}
				}
			}
			heightDistanceArray[2 * k] = dd;
			heightDistanceArray[2 * k + 1] = height;
			if (height != HEIGHT_UNDEFINED) {
				// interpolate undefined
				double totalDistance = dd;
				int startUndefined = k;
				while (startUndefined - 1 >= 0 &&
					   heightDistanceArray[2 * (startUndefined - 1) + 1] == HEIGHT_UNDEFINED) {
					startUndefined--;
					totalDistance += heightDistanceArray[2 * (startUndefined)];
				}
				if (totalDistance > 0) {
					double angle = (height - prevHeight) / totalDistance;
					for (int j = startUndefined; j < k; j++) {
						heightDistanceArray[2 * j + 1] =
							((heightDistanceArray[2 * j] * angle) + heightDistanceArray[2 * j - 1]);
					}
				}
				prevHeight = height;
			}

		} else {
			heightDistanceArray[0] = 0;
			heightDistanceArray[1] = startHeight;
		}
		plat = lat;
		plon = lon;
	}
	return heightDistanceArray;
}

string RouteDataObject::getHighway() {
	auto sz = types.size();
	for (int i = 0; i < sz; i++) {
		auto& r = region->routeEncodingRules[types[i]];
		if (r.getTag() == "highway") {
			return r.getValue();
		}
	}
	return "";
}

bool RouteDataObject::platform() {
	auto sz = types.size();
	for (int i = 0; i < sz; i++) {
		auto& r = region->routeEncodingRules[types[i]];
		if (r.getTag() == "railway" && r.getValue() == "platform") {
			return true;
		} else if (r.getTag() == "public_transport" && r.getValue() == "platform") {
			return true;
		}
	}
	return false;
}

bool RouteDataObject::roundabout() {
	auto sz = types.size();
	for (int i = 0; i < sz; i++) {
		auto& r = region->routeEncodingRules[types[i]];
		if (r.getTag() == "roundabout" || r.getValue() == "roundabout") {
			return true;
		} else if (r.getTag() == "oneway" && r.getValue() != "no" && loop()) {
			return true;
		}
	}
	return false;
}

void searchRouteSubRegion(int fileInd, std::vector<RouteDataObject*>& list, RoutingIndex* routingIndex,
						  RouteSubregion* sub);
void searchRouteRegion(CodedInputStream** input, FileInputStream** fis, BinaryMapFile* file, SearchQuery* q,
					   RoutingIndex* ind, std::vector<RouteSubregion>& subregions, std::vector<RouteSubregion>& toLoad,
					   bool geocoding);
bool readRouteTreeData(CodedInputStream* input, RouteSubregion* s, std::vector<RouteDataObject*>& dataObjects,
					   RoutingIndex* routingIndex);

bool sortRouteRegions(const RouteSubregion& i, const RouteSubregion& j) {
	return (i.mapDataBlock < j.mapDataBlock);
}

inline bool readInt(CodedInputStream* input, uint32_t* sz) {
	uint8_t buf[4];
	if (!input->ReadRaw(buf, 4)) {
		return false;
	}
	*sz = ((buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3]);
	return true;
}

bool skipFixed32(CodedInputStream* input) {
	uint32_t sz;
	if (!readInt(input, &sz)) {
		return false;
	}
	return input->Skip(sz);
}

bool skipUnknownFields(CodedInputStream* input, int tag) {
	if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_FIXED32_LENGTH_DELIMITED) {
		if (!skipFixed32(input)) {
			return false;
		}
	} else if (!WireFormatLite::SkipField(input, tag)) {
		return false;
	}
	return true;
}

bool readMapTreeBounds(CodedInputStream* input, MapTreeBounds* tree, MapRoot* root) {
	int init = 0;
	int tag;
	int32_t si;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kLeftFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				tree->left = si + root->left;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				tree->right = si + root->right;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				tree->top = si + root->top;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				tree->bottom = si + root->bottom;
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
		if (init == 0xf) {
			return true;
		}
	}
	return true;
}

bool readMapLevel(CodedInputStream* input, MapRoot* root, bool initSubtrees) {
	int tag;
	int si;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber: {
				int z;
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &z)));
				root->maxZoom = z;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber: {
				int z;
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &z)));
				root->minZoom = z;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &si)));
				root->bottom = si;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kTopFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &si)));
				root->top = si;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &si)));
				root->left = si;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kRightFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &si)));
				root->right = si;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber: {
				if (!initSubtrees) {
					input->Skip(input->BytesUntilLimit());
					break;
				}
				MapTreeBounds bounds;
				readInt(input, &bounds.length);
				bounds.filePointer = input->TotalBytesRead();
				int oldLimit = input->PushLimit(bounds.length);
				readMapTreeBounds(input, &bounds, root);
				root->bounds.push_back(bounds);
				input->PopLimit(oldLimit);
				break;
			}

			case OsmAnd::OBF::OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber: {
				input->Skip(input->BytesUntilLimit());
				break;
			}

			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

bool readRouteEncodingRule(CodedInputStream* input, RoutingIndex* index, uint32_t id) {
	int tag;
	std::string tagS;
	std::string value;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteEncodingRule::kValueFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &value)));
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteEncodingRule::kTagFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &tagS)));
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteEncodingRule::kIdFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &id)));
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	// Special case for check to not replace primary with primary_link
	index->initRouteEncodingRule(id, tagS, value);
	return true;
}

bool readMapEncodingRule(CodedInputStream* input, MapIndex* index, uint32_t id) {
	int tag;
	std::string tagS;
	std::string value;
	uint32_t type = 0;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::OsmAndMapIndex_MapEncodingRule::kValueFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &value)));
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapEncodingRule::kTagFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &tagS)));
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapEncodingRule::kTypeFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &type)));
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapEncodingRule::kIdFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &id)));
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	// Special case for check to not replace primary with primary_link
	index->initMapEncodingRule(type, id, tagS, value);
	return true;
}

bool readRouteTree(CodedInputStream* input, RouteSubregion* thisTree, RouteSubregion* parentTree, RoutingIndex* ind,
				   int depth, bool readCoordinates) {
	bool readChildren = depth != 0;
	uint32_t tag;
	int i = 0;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBox::kLeftFieldNumber: {
				WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &i);
				if (readCoordinates) {
					thisTree->left = i + (parentTree != NULL ? parentTree->left : 0);
				}
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBox::kRightFieldNumber: {
				WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &i);
				if (readCoordinates) {
					thisTree->right = i + (parentTree != NULL ? parentTree->right : 0);
				}
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBox::kTopFieldNumber: {
				WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &i);
				if (readCoordinates) {
					thisTree->top = i + (parentTree != NULL ? parentTree->top : 0);
				}
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBox::kBottomFieldNumber: {
				WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &i);
				if (readCoordinates) {
					thisTree->bottom = i + (parentTree != NULL ? parentTree->bottom : 0);
				}
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBox::kShiftToDataFieldNumber: {
				readInt(input, &thisTree->mapDataBlock);
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBox::kBoxesFieldNumber: {
				if (readChildren) {
					RouteSubregion subregion(ind);
					readInt(input, &subregion.length);
					subregion.filePointer = input->TotalBytesRead();
					int oldLimit = input->PushLimit(subregion.length);
					readRouteTree(input, &subregion, thisTree, ind, depth - 1, true);
					input->PopLimit(oldLimit);
					input->Seek(subregion.filePointer + subregion.length);
					thisTree->subregions.push_back(subregion);
				} else {
					if (!skipUnknownFields(input, tag)) {
						return false;
					}
				}
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

bool readRoutingIndex(CodedInputStream* input, RoutingIndex* routingIndex, bool readOnlyRules) {
	uint32_t defaultId = 1;
	uint32_t tag;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::OsmAndRoutingIndex::kNameFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &routingIndex->name)));
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex::kRulesFieldNumber: {
				int len = 0;
				WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &len);
				int oldLimit = input->PushLimit(len);
				readRouteEncodingRule(input, routingIndex, defaultId++);
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex::kRootBoxesFieldNumber:
			case OsmAnd::OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber: {
				if (readOnlyRules) {
					input->Seek(routingIndex->filePointer + routingIndex->length);
					break;
				}
				bool basemap =
					WireFormatLite::GetTagFieldNumber(tag) == OsmAnd::OBF::OsmAndRoutingIndex::kBasemapBoxesFieldNumber;
				RouteSubregion subregion(routingIndex);
				readInt(input, &subregion.length);
				subregion.filePointer = input->TotalBytesRead();
				int oldLimit = input->PushLimit(subregion.length);
				readRouteTree(input, &subregion, NULL, routingIndex, 0, true);
				input->PopLimit(oldLimit);
				input->Seek(subregion.filePointer + subregion.length);
				if (basemap) {
					routingIndex->basesubregions.push_back(subregion);
				} else {
					routingIndex->subregions.push_back(subregion);
				}
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex::kBlocksFieldNumber: {
				// Finish reading
				input->Seek(routingIndex->filePointer + routingIndex->length);
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	routingIndex->completeRouteEncodingRules();
	return true;
}

bool readTransportBounds(CodedInputStream* input, TransportIndex* ind) {
	while (true) {
		int si;
		int tag = input->ReadTag();
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case 0: {
				return true;
			}
			case OsmAnd::OBF::TransportStopsTree::kLeftFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				ind->left = si;
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kRightFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				ind->right = si;
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kTopFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				ind->top = si;
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kBottomFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				ind->bottom = si;
				break;
			}
			default: {
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
}

bool readTransportIndex(CodedInputStream* input, TransportIndex* ind) {
	while (true) {
		int tag = input->ReadTag();
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case 0: {
				return true;
			}
			case OsmAnd::OBF::OsmAndTransportIndex::kRoutesFieldNumber: {
				skipUnknownFields(input, tag);
				break;
			}
			case OsmAnd::OBF::OsmAndTransportIndex::kNameFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &ind->name)));
				break;
			}
			case OsmAnd::OBF::OsmAndTransportIndex::kStopsFieldNumber: {
				readInt(input, &ind->stopsFileLength);
				ind->stopsFileOffset = input->TotalBytesRead();
				int old = input->PushLimit(ind->stopsFileLength);
				readTransportBounds(input, &(*ind));
				input->PopLimit(old);
				break;
			}
			case OsmAnd::OBF::OsmAndTransportIndex::kStringTableFieldNumber: {
				IndexStringTable* st = new IndexStringTable();
				input->ReadVarint32(&st->length);

				st->fileOffset = input->TotalBytesRead();
				ind->stringTable = st;
				input->Seek(st->length + st->fileOffset);
				break;
			}
			case OsmAnd::OBF::OsmAndTransportIndex::kIncompleteRoutesFieldNumber: {
				input->ReadVarint32(&ind->incompleteRoutesLength);
				ind->incompleteRoutesOffset = input->TotalBytesRead();
				input->Seek(ind->incompleteRoutesOffset + ind->incompleteRoutesLength);
				break;
			}
			default: {
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
}

bool readMapIndex(CodedInputStream* input, MapIndex* mapIndex, bool onlyInitEncodingRules) {
	uint32_t tag;
	uint32_t defaultId = 1;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::OsmAndMapIndex::kNameFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &mapIndex->name)));
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex::kRulesFieldNumber: {
				if (onlyInitEncodingRules) {
					int len = 0;
					WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &len);
					int oldLimit = input->PushLimit(len);
					readMapEncodingRule(input, mapIndex, defaultId++);
					input->PopLimit(oldLimit);
				} else {
					skipUnknownFields(input, tag);
				}
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex::kLevelsFieldNumber: {
				MapRoot mapLevel;
				readInt(input, &mapLevel.length);
				mapLevel.filePointer = input->TotalBytesRead();
				if (!onlyInitEncodingRules) {
					int oldLimit = input->PushLimit(mapLevel.length);
					readMapLevel(input, &mapLevel, false);
					input->PopLimit(oldLimit);
					mapIndex->levels.push_back(mapLevel);
				}
				input->Seek(mapLevel.filePointer + mapLevel.length);
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	if (onlyInitEncodingRules) {
		mapIndex->finishInitializingTags();
	}
	return true;
}

bool initMapStructure(CodedInputStream* input, BinaryMapFile* file, bool useLive, bool initRoutingOnly) {
	uint32_t tag;
	uint32_t versionConfirm = -2;
	file->external = file->inputName.find("osmand_ext") != string::npos;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			// required uint32_t version = 1;
			case OsmAnd::OBF::OsmAndStructure::kVersionFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &file->version)));
				break;
			}
			case OsmAnd::OBF::OsmAndStructure::kDateCreatedFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint64_t, WireFormatLite::TYPE_UINT64>(input, &file->dateCreated)));
				break;
			}
			case OsmAnd::OBF::OsmAndStructure::kMapIndexFieldNumber: {
				MapIndex mapIndex;
				readInt(input, &mapIndex.length);
				mapIndex.filePointer = input->TotalBytesRead();
				int oldLimit = input->PushLimit(mapIndex.length);
				if (!initRoutingOnly) {
					readMapIndex(input, &mapIndex, false);
				}
				input->PopLimit(oldLimit);
				input->Seek(mapIndex.filePointer + mapIndex.length);
				file->mapIndexes.push_back(mapIndex);
				file->indexes.push_back(&file->mapIndexes.back());
				file->basemap = file->basemap || mapIndex.name.find("basemap") != string::npos;
				file->external = file->external || mapIndex.name.find("osmand_ext") != string::npos;
				break;
			}
			case OsmAnd::OBF::OsmAndStructure::kRoutingIndexFieldNumber: {
				RoutingIndex* routingIndex = new RoutingIndex;
				readInt(input, &routingIndex->length);
				routingIndex->filePointer = input->TotalBytesRead();
				int oldLimit = input->PushLimit(routingIndex->length);
				readRoutingIndex(input, routingIndex, false);
				input->PopLimit(oldLimit);
				input->Seek(routingIndex->filePointer + routingIndex->length);
				if (!file->liveMap || useLive) {
					file->routingIndexes.push_back(routingIndex);
					file->indexes.push_back(file->routingIndexes.back());
				}
				break;
			}
			case OsmAnd::OBF::OsmAndStructure::kTransportIndexFieldNumber: {
				TransportIndex* tIndex = new TransportIndex();
				readInt(input, &tIndex->length);
				tIndex->filePointer = input->TotalBytesRead();

				int oldLimit = input->PushLimit(tIndex->length);
				readTransportIndex(input, tIndex);
				input->PopLimit(oldLimit);
				file->transportIndexes.push_back(tIndex);
				file->indexes.push_back(tIndex);

				input->Seek(tIndex->filePointer + tIndex->length);
				break;
			}
			case OsmAnd::OBF::OsmAndStructure::kVersionConfirmFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &versionConfirm)));
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	if (file->version != versionConfirm) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error,
						  "Corrupted file. It should be ended as it starts with version");
		return false;
	}
	if (file->version != MAP_VERSION) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Version of the file is not supported.");
		return false;
	}
	return true;
}

bool readStringTable(CodedInputStream* input, std::vector<std::string>& list) {
	uint32_t tag;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::StringTable::kSFieldNumber: {
				std::string s;
				WireFormatLite::ReadString(input, &s);
				list.push_back(s);
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return false;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

static const int ROUTE_SHIFT_COORDINATES = 4;
static const int MASK_TO_READ = ~((1 << SHIFT_COORDINATES) - 1);

bool acceptTypes(SearchQuery* req, std::vector<tag_value>& types, MapIndex* root) {
	RenderingRuleSearchRequest* r = req->req;
	for (std::vector<tag_value>::iterator type = types.begin(); type != types.end(); type++) {
		for (int i = 1; i <= 3; i++) {
			r->setIntFilter(r->props()->R_MINZOOM, req->zoom);
			r->setStringFilter(r->props()->R_TAG, type->first);
			r->setStringFilter(r->props()->R_VALUE, type->second);
			if (r->search(i, false)) {
				return true;
			}
		}
		r->setStringFilter(r->props()->R_TAG, type->first);
		r->setStringFilter(r->props()->R_VALUE, type->second);
		r->setStringFilter(r->props()->R_NAME_TAG, "");
		if (r->search(RenderingRulesStorage::TEXT_RULES, false)) {
			return true;
		}
	}

	return false;
}

MapDataObject* readMapDataObject(CodedInputStream* input, MapTreeBounds* tree, SearchQuery* req, MapIndex* root,
								 uint64_t baseId) {
	uint32_t tag = WireFormatLite::GetTagFieldNumber(input->ReadTag());
	bool area = (uint32_t)OsmAnd::OBF::MapData::kAreaCoordinatesFieldNumber == tag;
	if (!area && (uint32_t)OsmAnd::OBF::MapData::kCoordinatesFieldNumber != tag) {
		return NULL;
	}
	req->cacheCoordinates.clear();
	uint32_t size;
	input->ReadVarint32(&size);
	int old = input->PushLimit(size);
	int px = tree->left & MASK_TO_READ;
	int py = tree->top & MASK_TO_READ;
	bool contains = false;
	int64_t id = 0;
	int minX = INT_MAXIMUM;
	int maxX = 0;
	int minY = INT_MAXIMUM;
	int maxY = 0;
	req->numberOfVisitedObjects++;
	int x;
	int y;
	while (input->BytesUntilLimit() > 0) {
		if (!WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &x)) {
			return NULL;
		}
		if (!WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &y)) {
			return NULL;
		}
		x = (x << SHIFT_COORDINATES) + px;
		y = (y << SHIFT_COORDINATES) + py;
		req->cacheCoordinates.push_back(std::pair<int, int>(x, y));
		px = x;
		py = y;
		if (!contains && req->left <= x && req->right >= x && req->top <= y && req->bottom >= y) {
			contains = true;
		}
		if (!contains) {
			minX = std::min(minX, x);
			maxX = std::max(maxX, x);
			minY = std::min(minY, y);
			maxY = std::max(maxY, y);
		}
	}
	if (!contains) {
		if (maxX >= req->left && minX <= req->right && minY <= req->bottom && maxY >= req->top) {
			contains = true;
		}
	}
	input->PopLimit(old);
	if (!contains) {
		return NULL;
	}

	// READ types
	std::vector<coordinates> innercoordinates;
	std::vector<tag_value> additionalTypes;
	std::vector<tag_value> types;
	UNORDERED(map)<std::string, unsigned int> stringIds;
	std::vector<std::string> namesOrder;
	int32_t labelX = 0;
	int32_t labelY = 0;
	bool loop = true;
	while (loop) {
		uint32_t t = input->ReadTag();
		switch (WireFormatLite::GetTagFieldNumber(t)) {
			case 0:
				loop = false;
				break;
			case OsmAnd::OBF::MapData::kPolygonInnerCoordinatesFieldNumber: {
				coordinates polygon;

				px = tree->left & MASK_TO_READ;
				py = tree->top & MASK_TO_READ;
				input->ReadVarint32(&size);
				old = input->PushLimit(size);
				while (input->BytesUntilLimit() > 0) {
					WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &x);
					WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &y);
					x = (x << SHIFT_COORDINATES) + px;
					y = (y << SHIFT_COORDINATES) + py;
					polygon.push_back(std::pair<int, int>(x, y));
					px = x;
					py = y;
				}
				input->PopLimit(old);
				innercoordinates.push_back(polygon);
				break;
			}
			case OsmAnd::OBF::MapData::kAdditionalTypesFieldNumber: {
				input->ReadVarint32(&size);
				old = input->PushLimit(size);
				while (input->BytesUntilLimit() > 0) {
					WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &x);
					if (root->decodingRules.find(x) != root->decodingRules.end()) {
						tag_value t = root->decodingRules[x];
						additionalTypes.push_back(t);
					}
				}
				input->PopLimit(old);
				break;
			}
			case OsmAnd::OBF::MapData::kTypesFieldNumber: {
				input->ReadVarint32(&size);
				old = input->PushLimit(size);
				while (input->BytesUntilLimit() > 0) {
					WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &x);
					if (root->decodingRules.find(x) != root->decodingRules.end()) {
						tag_value t = root->decodingRules[x];
						types.push_back(t);
					}
				}
				input->PopLimit(old);
				// bool acceptTps = acceptTypes(req, types, root);
				// if (!acceptTps) {
				//	return NULL;
				//}
				break;
			}
			case OsmAnd::OBF::MapData::kIdFieldNumber:
				WireFormatLite::ReadPrimitive<int64_t, WireFormatLite::TYPE_SINT64>(input, &id);
				break;
			case OsmAnd::OBF::MapData::kStringNamesFieldNumber:
				input->ReadVarint32(&size);
				old = input->PushLimit(size);
				while (input->BytesUntilLimit() > 0) {
					WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &x);
					WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &y);
					if (root->decodingRules.find(x) != root->decodingRules.end()) {
						tag_value t = root->decodingRules[x];
						stringIds[t.first] = y;
						namesOrder.push_back(t.first);
					}
				}
				input->PopLimit(old);
				break;

			case OsmAnd::OBF::MapData::kLabelcoordinatesFieldNumber: {
				input->ReadVarint32(&size);
				old = input->PushLimit(size);
				u_int i = 0;
				while (input->BytesUntilLimit() > 0) {
					if (i == 0) {
						WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &labelX);
					} else if (i == 1) {
						WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &labelY);
					} else {
						WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_INT32>(input, &x);
					}
					i++;
				}
				input->PopLimit(old);
				break;
			}

			default: {
				if (WireFormatLite::GetTagWireType(t) == WireFormatLite::WIRETYPE_END_GROUP) {
					return NULL;
				}
				if (!skipUnknownFields(input, t)) {
					return NULL;
				}
				break;
			}
		}
	}
	//	if(req->cacheCoordinates.size() > 100 && types.size() > 0 /*&& types[0].first == "admin_level"*/) {
	//		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "TODO Object is %llu  (%llu) ignored %s %s", (id + baseId)
	//>> 1, baseId, types[0].first.c_str(), types[0].second.c_str()); 		return NULL;
	//	}

	req->numberOfAcceptedObjects++;

	MapDataObject* dataObject = new MapDataObject();
	dataObject->points = req->cacheCoordinates;
	dataObject->additionalTypes = additionalTypes;
	dataObject->types = types;
	dataObject->id = id;
	dataObject->area = area;
	dataObject->stringIds = stringIds;
	dataObject->namesOrder = namesOrder;
	dataObject->polygonInnerCoordinates = innercoordinates;
	dataObject->labelX = labelX;
	dataObject->labelY = labelY;
	return dataObject;
}

//------ Transport Index Reading-----------------

string regStr(UNORDERED(map) < int32_t, string > &stringTable, CodedInputStream* input) {
	uint32_t i = 0;
	WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &i);
	stringTable.insert({i, ""});
	return to_string(i);
}

string regStr(UNORDERED(map) < int32_t, string > &stringTable, int32_t i) {
	stringTable.insert({i, ""});
	return to_string(i);
}

bool readIncompleteRoute(CodedInputStream* input, shared_ptr<IncompleteTransportRoute>& obj, int transportIndexOffset) {
	bool end = false;
	while (!end) {
		int t = input->ReadTag();
		int tag = WireFormatLite::GetTagFieldNumber(t);
		switch (tag) {
			case OsmAnd::OBF::IncompleteTransportRoute::kIdFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint64_t, WireFormatLite::TYPE_UINT64>(input, &obj->routeId)));
				break;
			}
			case OsmAnd::OBF::IncompleteTransportRoute::kRouteRefFieldNumber: {
				uint32_t shift;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &shift)));
				if (shift > transportIndexOffset) {
					obj->routeOffset = shift;
				} else {
					obj->routeOffset = transportIndexOffset + shift;
				}
				break;
			}
			case OsmAnd::OBF::IncompleteTransportRoute::kOperatorFieldNumber: {
				skipUnknownFields(input, t);
				break;
			}
			case OsmAnd::OBF::IncompleteTransportRoute::kRefFieldNumber: {
				skipUnknownFields(input, t);
				break;
			}
			case OsmAnd::OBF::IncompleteTransportRoute::kTypeFieldNumber: {
				skipUnknownFields(input, t);
				break;
			}
			case OsmAnd::OBF::IncompleteTransportRoute::kMissingStopsFieldNumber: {
				skipUnknownFields(input, t);
				break;
			}
			case 0: {
				end = true;
				return true;
				break;
			}
			default: {
				if (!skipUnknownFields(input, t)) {
					return false;
				}
				break;
			}
		}
	}
	return false;
}

void readIncompleteRoutesList(CodedInputStream* input, UNORDERED(map) < uint64_t,
							  shared_ptr<IncompleteTransportRoute> > &incompleteRoutes, int transportIndexOffset) {
	bool end = false;
	uint32_t sizeL;
	while (!end) {
		int t = input->ReadTag();
		int tag = WireFormatLite::GetTagFieldNumber(t);
		switch (tag) {
			case OsmAnd::OBF::IncompleteTransportRoutes::kRoutesFieldNumber: {
				input->ReadVarint32(&sizeL);
				int32_t olds = input->PushLimit(sizeL);
				shared_ptr<IncompleteTransportRoute> ir = make_shared<IncompleteTransportRoute>();
				readIncompleteRoute(input, ir, transportIndexOffset);

				if (incompleteRoutes.find(ir->routeId) != incompleteRoutes.end()) {
					incompleteRoutes[ir->routeId]->setNextLinkedRoute(ir);
				} else {
					incompleteRoutes.insert({ir->routeId, ir});
				}
				input->PopLimit(olds);
				break;
			}
			case 0: {
				end = true;
				break;
			}
			default: {
				skipUnknownFields(input, t);
				break;
			}
		}
	}
}

void getIncompleteTransportRoutes(BinaryMapFile* file) {
	if (!file->incompleteLoaded) {
		for (auto& ti : file->transportIndexes) {
			if (ti->incompleteRoutesLength > 0) {
				lseek(file->routefd, 0, SEEK_SET);
				FileInputStream stream(file->routefd);
				stream.SetCloseOnDelete(false);
				CodedInputStream* input = new CodedInputStream(&stream);
				input->SetTotalBytesLimit(INT_MAX, INT_MAX >> 1);

				input->Seek(ti->incompleteRoutesOffset);
				int oldLimit = input->PushLimit(ti->incompleteRoutesLength);
				readIncompleteRoutesList(input, file->incompleteTransportRoutes, ti->filePointer);
				input->PopLimit(oldLimit);
			}
		}
		file->incompleteLoaded = true;
	}
}

bool readTransportStopExit(CodedInputStream* input, SHARED_PTR<TransportStopExit>& exit, int cleft, int ctop,
						   SearchQuery* req, UNORDERED(map) < int32_t, string > &stringTable) {
	int32_t x = 0;
	int32_t y = 0;

	while (true) {
		int tag = WireFormatLite::GetTagFieldNumber(input->ReadTag());
		switch (tag) {
			case OsmAnd::OBF::TransportStopExit::kRefFieldNumber:
				exit->ref = regStr(stringTable, input);
				break;
			case OsmAnd::OBF::TransportStopExit::kDxFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &x)));
				x += cleft;
				break;
			}
			case OsmAnd::OBF::TransportStopExit::kDyFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &y)));
				y += ctop;
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					// if (dataObject->getName("en").length() == 0) {
					// 	dataObject->enName(TransliterationHelper.transliterate(dataObject.getName()));
					// }
					if (x != 0 || y != 0) {
						exit->setLocation(TRANSPORT_STOP_ZOOM, x, y);
					}
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
}

bool readTransportStop(int stopOffset, SHARED_PTR<TransportStop>& stop, CodedInputStream* input, int pleft, int pright,
					   int ptop, int pbottom, SearchQuery* req, UNORDERED(map) < int32_t, string > &stringTable) {
	uint32_t tag = WireFormatLite::GetTagFieldNumber(input->ReadTag());
	if (OsmAnd::OBF::TransportStop::kDxFieldNumber != tag) {
		return false;
	}
	int32_t x = 0;
	DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &x)));
	x += pleft;

	tag = WireFormatLite::GetTagFieldNumber(input->ReadTag());
	if (OsmAnd::OBF::TransportStop::kDyFieldNumber != tag) {
		return false;
	}

	int32_t y = 0;
	DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &y)));
	y += ptop;
	if (req->right < x || req->left > x || req->top > y || req->bottom < y) {
		input->Skip(input->BytesUntilLimit());
		return false;
	}

	req->numberOfAcceptedObjects++;
	req->cacheTypes.clear();
	req->cacheIdsA.clear();
	req->cacheIdsB.clear();
	uint32_t si32;
	uint64_t si64;
	stop->setLocation(TRANSPORT_STOP_ZOOM, x, y);
	stop->fileOffset = stopOffset;
	while (true) {
		int t = input->ReadTag();
		tag = WireFormatLite::GetTagFieldNumber(t);
		switch (tag) {
			case OsmAnd::OBF::TransportStop::kRoutesFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &si32)));
				req->cacheTypes.push_back(stopOffset - si32);
				break;
			}
			case OsmAnd::OBF::TransportStop::kDeletedRoutesIdsFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint64_t, WireFormatLite::TYPE_UINT64>(input, &si64)));
				req->cacheIdsA.push_back(si64);
				break;
			}
			case OsmAnd::OBF::TransportStop::kRoutesIdsFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint64_t, WireFormatLite::TYPE_UINT64>(input, &si64)));
				req->cacheIdsB.push_back(si64);
				break;
			}
			case OsmAnd::OBF::TransportStop::kNameEnFieldNumber: {
				stop->enName = regStr(stringTable, input);
				break;
			}
			case OsmAnd::OBF::TransportStop::kNameFieldNumber: {
				stop->name = regStr(stringTable, input);
				break;
			}
			case OsmAnd::OBF::TransportStop::kAdditionalNamePairsFieldNumber: {
				// if (stringTable.get()) {
				uint32_t sizeL;
				input->ReadVarint32(&sizeL);
				int32_t oldRef = input->PushLimit(sizeL);
				while (input->BytesUntilLimit() > 0) {
					uint32_t l;
					uint32_t n;
					input->ReadVarint32(&l);
					input->ReadVarint32(&n);

					stop->names.insert({regStr(stringTable, l), regStr(stringTable, n)});
				}
				input->PopLimit(oldRef);
				// } else {
				// 	skipUnknownFields(input, t);
				// }
				break;
			}
			case OsmAnd::OBF::TransportStop::kIdFieldNumber: {
				int64_t id;
				DO_((WireFormatLite::ReadPrimitive<int64_t, WireFormatLite::TYPE_SINT64>(input, &id)));
				stop->id = id;
				break;
			}
			case OsmAnd::OBF::TransportStop::kExitsFieldNumber: {
				uint32_t length;
				input->ReadVarint32(&length);
				int oldLimit = input->PushLimit(length);
				SHARED_PTR<TransportStopExit> transportStopExit = make_shared<TransportStopExit>();
				readTransportStopExit(input, transportStopExit, pleft, ptop, req, stringTable);
				stop->exits.push_back(transportStopExit);
				input->PopLimit(oldLimit);
				break;
			}
			case 0: {
				stop->referencesToRoutes = req->cacheTypes;
				stop->deletedRoutesIds = req->cacheIdsA;
				stop->routesIds = req->cacheIdsB;
				// if(dataObject->names.find("en").length() == 0){
				//     dataObject->enName = TransliterationHelper.transliterate(dataObject->name);
				// }
				return true;
			}
			default: {
				if (!skipUnknownFields(input, t)) {
					return false;
				}
				break;
			}
		}
	}
}

bool searchTransportTreeBounds(CodedInputStream* input, int pleft, int pright, int ptop, int pbottom, SearchQuery* req,
							   UNORDERED(map) < int32_t, string > &stringTable) {
	int init = 0;
	int lastIndexResult = -1;
	int cright = 0;
	int cleft = 0;
	int ctop = 0;
	int cbottom = 0;
	int si;

	req->numberOfReadSubtrees++;
	int tag;
	while ((tag = input->ReadTag()) != 0) {
		if (req->publisher->isCancelled()) {
			return false;
		}

		if (init == 0xf) {
			init = 0;
			// coordinates are init
			if (cright < req->left || cleft > req->right || ctop > req->bottom || cbottom < req->top) {
				return false;
			} else {
				req->numberOfAcceptedSubtrees++;
			}
		}

		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::TransportStopsTree::kBottomFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				cbottom = si + pbottom;
				init |= 1;
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kLeftFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				cleft = si + pleft;
				init |= 2;
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kRightFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				cright = si + pright;
				init |= 4;
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kTopFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				ctop = si + ptop;
				init |= 8;
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kLeafsFieldNumber: {
				int stopOffset = input->TotalBytesRead();
				uint32_t length = 0;
				input->ReadVarint32(&length);
				int oldLimit = input->PushLimit(length);

				if (lastIndexResult == -1) {
					lastIndexResult = req->transportResults.size();
				}
				req->numberOfVisitedObjects++;
				SHARED_PTR<TransportStop> transportStop = make_shared<TransportStop>();
				if (readTransportStop(stopOffset, transportStop, input, cleft, cright, ctop, cbottom, req, stringTable))
					req->transportResults.push_back(transportStop);

				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kSubtreesFieldNumber: {
				uint32_t length = 0;
				readInt(input, &length);
				int filePointer = input->TotalBytesRead();
				if (req->limit == -1 || req->limit >= req->transportResults.size()) {
					int oldLimit = input->PushLimit(length);
					searchTransportTreeBounds(input, cleft, cright, ctop, cbottom, req, stringTable);
					input->PopLimit(oldLimit);
				}
				input->Seek(filePointer + length);

				if (lastIndexResult >= 0) {
					// 	throw new IllegalStateException();
				}
				break;
			}
			case OsmAnd::OBF::TransportStopsTree::kBaseIdFieldNumber: {
				uint64_t baseId = 0;
				DO_((WireFormatLite::ReadPrimitive<uint64_t, WireFormatLite::TYPE_UINT64>(input, &baseId)));
				if (lastIndexResult != -1) {
					for (int i = lastIndexResult; i < req->transportResults.size(); i++) {
						const auto rs = req->transportResults[i];
						rs->id = rs->id + baseId;
					}
				}
				break;
			}
			default: {
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

bool readTransportSchedule(CodedInputStream* input, TransportSchedule& schedule) {
	while (true) {
		uint32_t sizeL, interval;
		int32_t old;
		int t = input->ReadTag();
		int tag = WireFormatLite::GetTagFieldNumber(t);
		switch (tag) {
			case 0:
				return true;
			case OsmAnd::OBF::TransportRouteSchedule::kTripIntervalsFieldNumber:
				input->ReadVarint32(&sizeL);
				old = input->PushLimit(sizeL);
				while (input->BytesUntilLimit() > 0) {
					input->ReadVarint32(&interval);
					schedule.tripIntervals.push_back(interval);
				}
				input->PopLimit(old);
				break;
			case OsmAnd::OBF::TransportRouteSchedule::kAvgStopIntervalsFieldNumber:
				input->ReadVarint32(&sizeL);
				old = input->PushLimit(sizeL);
				while (input->BytesUntilLimit() > 0) {
					input->ReadVarint32(&interval);
					schedule.avgStopIntervals.push_back(interval);
				}
				input->PopLimit(old);
				break;
			case OsmAnd::OBF::TransportRouteSchedule::kAvgWaitIntervalsFieldNumber:
				input->ReadVarint32(&sizeL);
				old = input->PushLimit(sizeL);
				while (input->BytesUntilLimit() > 0) {
					input->ReadVarint32(&interval);
					schedule.avgWaitIntervals.push_back(interval);
				}
				input->PopLimit(old);
				break;
			default: {
				if (!skipUnknownFields(input, t)) {
					return false;
				}
				break;
			}
		}
	}
}

bool readTransportRouteStop(CodedInputStream* input, SHARED_PTR<TransportStop>& transportStop, int dx[], int dy[],
							int64_t did, UNORDERED(map) < int32_t, string > &stringTable, int32_t filePointer) {
	transportStop->fileOffset = input->TotalBytesRead();
	transportStop->referencesToRoutes.push_back(filePointer);
	bool end = false;
	int32_t sm = 0;
	int64_t tm = 0;
	while (!end) {
		int t = input->ReadTag();
		int tag = WireFormatLite::GetTagFieldNumber(t);
		switch (tag) {
			case OsmAnd::OBF::TransportRouteStop::kNameEnFieldNumber:
				transportStop->enName = regStr(stringTable, input);
				break;
			case OsmAnd::OBF::TransportRouteStop::kNameFieldNumber:
				transportStop->name = regStr(stringTable, input);
				break;
			case OsmAnd::OBF::TransportRouteStop::kIdFieldNumber:
				DO_((WireFormatLite::ReadPrimitive<int64_t, WireFormatLite::TYPE_SINT64>(input, &tm)));
				did += tm;
				break;
			case OsmAnd::OBF::TransportRouteStop::kDxFieldNumber:
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &sm)));
				dx[0] += sm;
				break;
			case OsmAnd::OBF::TransportRouteStop::kDyFieldNumber:
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &sm)));
				dy[0] += sm;
				break;
			case 0:
				end = true;
				break;
			default: {
				if (!skipUnknownFields(input, t)) {
					return false;
				}
				break;
			}
		}
	}
	transportStop->id = did;
	transportStop->setLocation(TRANSPORT_STOP_ZOOM, dx[0], dy[0]);
	return true;
}

bool readTransportRoute(BinaryMapFile* file, SHARED_PTR<TransportRoute>& transportRoute, int32_t filePointer,
						UNORDERED(map) < int32_t, string > &stringTable, bool onlyDescription) {
	lseek(file->routefd, 0, SEEK_SET);
	FileInputStream stream(file->routefd);
	stream.SetCloseOnDelete(false);
	CodedInputStream* input = new CodedInputStream(&stream);
	input->SetTotalBytesLimit(INT_MAX, INT_MAX >> 1);
	input->Seek(filePointer);

	uint32_t routeLength;
	input->ReadVarint32(&routeLength);
	int old = input->PushLimit(routeLength);
	transportRoute->fileOffset = filePointer;
	bool end = false;
	int64_t rid = 0;
	int rx[] = {0};
	int ry[] = {0};
	uint32_t sizeL;
	int32_t pold;
	while (!end) {
		int t = input->ReadTag();
		int tag = WireFormatLite::GetTagFieldNumber(t);
		switch (tag) {
			case 0: {
				end = true;
				break;
			}
			case OsmAnd::OBF::TransportRoute::kDistanceFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input,
																						  &transportRoute->dist)));
				break;
			}
			case OsmAnd::OBF::TransportRoute::kIdFieldNumber: {
				uint64_t i;
				DO_((WireFormatLite::ReadPrimitive<uint64_t, WireFormatLite::TYPE_UINT64>(input, &i)));
				transportRoute->id = i;
				break;
			}
			case OsmAnd::OBF::TransportRoute::kRefFieldNumber: {
				DO_((WireFormatLite::ReadString(input, &transportRoute->ref)));
				break;
			}
			case OsmAnd::OBF::TransportRoute::kTypeFieldNumber: {
				transportRoute->type = regStr(stringTable, input);
				break;
			}
			case OsmAnd::OBF::TransportRoute::kNameEnFieldNumber: {
				transportRoute->enName = regStr(stringTable, input);
				break;
			}
			case OsmAnd::OBF::TransportRoute::kNameFieldNumber: {
				transportRoute->name = regStr(stringTable, input);
				break;
			}
			case OsmAnd::OBF::TransportRoute::kOperatorFieldNumber:
				transportRoute->routeOperator = regStr(stringTable, input);
				break;
			case OsmAnd::OBF::TransportRoute::kColorFieldNumber: {
				transportRoute->color = regStr(stringTable, input);
				break;
			}
			case OsmAnd::OBF::TransportRoute::kGeometryFieldNumber: {
				input->ReadVarint32(&sizeL);
				pold = input->PushLimit(sizeL);
				int px = 0;
				int py = 0;
				shared_ptr<Way> w = make_shared<Way>();
				while (input->BytesUntilLimit() > 0) {
					int32_t ddx;
					int32_t ddy;
					DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &ddx)));
					DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &ddy)));
					ddx = ddx << SHIFT_COORDINATES;
					ddy = ddy << SHIFT_COORDINATES;
					if (ddx == 0 && ddy == 0) {
						if (w->nodes.size() > 0) {
							transportRoute->addWay(w);
						}
						w = make_shared<Way>();
					} else {
						int x = ddx + px;
						int y = ddy + py;
						Node n(get31LatitudeY(y), get31LongitudeX(x));
						w->addNode(n);
						px = x;
						py = y;
					}
				}
				if (w->nodes.size() > 0) {
					transportRoute->addWay(w);
				}
				input->PopLimit(pold);
				break;
			}
			case OsmAnd::OBF::TransportRoute::kScheduleTripFieldNumber: {
				input->ReadVarint32(&sizeL);
				pold = input->PushLimit(sizeL);
				readTransportSchedule(input, transportRoute->getOrCreateSchedule());
				input->PopLimit(pold);
				break;
			}
			case OsmAnd::OBF::TransportRoute::kDirectStopsFieldNumber: {
				if (onlyDescription) {
					end = true;
					input->Skip(input->BytesUntilLimit());
					break;
				}
				uint32_t length;
				input->ReadVarint32(&length);
				pold = input->PushLimit(length);
				SHARED_PTR<TransportStop> stop = make_shared<TransportStop>();
				readTransportRouteStop(input, stop, rx, ry, rid, stringTable, filePointer);
				transportRoute->forwardStops.push_back(stop);
				rid = stop->id;
				input->PopLimit(pold);
				break;
			}
			default: {
				if (!skipUnknownFields(input, t)) {
					return false;
				}
				break;
			}
		}
	}
	input->PopLimit(old);
	return true;
}

bool initializeStringTable(CodedInputStream* input, TransportIndex* ind, UNORDERED(map) < int32_t,
						   string > &requested) {
	if (ind->stringTable->stringTable.size() == 0) {
		input->Seek(ind->stringTable->fileOffset);
		int oldLimit = input->PushLimit(ind->stringTable->length);
		int current = 0;
		bool end = false;
		while (!end) {
			int t = input->ReadTag();
			int tag = WireFormatLite::GetTagFieldNumber(t);
			switch (tag) {
				case 0:
					end = true;
					break;
				case OsmAnd::OBF::StringTable::kSFieldNumber: {
					string value;
					DO_((WireFormatLite::ReadString(input, &value)));
					ind->stringTable->stringTable.insert({current, value});
					current++;
					break;
				}
				default: {
					if (!skipUnknownFields(input, t)) {
						return false;
					}
					break;
				}
			}
		}
		input->PopLimit(oldLimit);
	}
	return true;
}

void initializeNames(UNORDERED(map) < int32_t, string > &stringTable, SHARED_PTR<TransportStop> s) {
	for (SHARED_PTR<TransportStopExit>& exit : s->exits) {
		if (exit->ref.size() > 0) {
			const auto it = stringTable.find(atoi(exit->ref.c_str()));
			exit->ref = it != stringTable.end() ? it->second : "";
		}
	}
	if (s->name.size() > 0) {
		const auto it = stringTable.find(atoi(s->name.c_str()));
		s->name = it != stringTable.end() ? it->second : "";
	}
	if (s->enName.size() > 0) {
		const auto it = stringTable.find(atoi(s->enName.c_str()));
		s->enName = it != stringTable.end() ? it->second : "";
	}
	UNORDERED(map)<string, string> namesMap;
	if (s->names.size() > 0) {
		namesMap.insert(s->names.begin(), s->names.end());
		s->names.clear();
		UNORDERED(map)<string, string>::iterator it = namesMap.begin();
		while (it != namesMap.end()) {
			const auto first = stringTable.find(atoi(it->first.c_str()));
			const auto second = stringTable.find(atoi(it->second.c_str()));
			if (first != stringTable.end() && second != stringTable.end())
				s->names.insert({first->second, second->second});
			it++;
		}
	}
}

void initializeNames(bool onlyDescription, SHARED_PTR<TransportRoute>& dataObject, UNORDERED(map) < int32_t,
					 string > &stringTable) {
	if (dataObject->name.size() > 0) {
		const auto it = stringTable.find(atoi(dataObject->name.c_str()));
		dataObject->name = it != stringTable.end() ? it->second : "";
	}
	if (dataObject->enName.size() > 0) {
		const auto it = stringTable.find(atoi(dataObject->enName.c_str()));
		dataObject->enName = it != stringTable.end() ? it->second : "";
	}
	// if(dataObject->getName().length() > 0 && dataObject.getName("en").length() == 0){
	// 	dataObject.setEnName(TransliterationHelper.transliterate(dataObject.getName()));
	// }
	if (dataObject->routeOperator.size() > 0) {
		const auto it = stringTable.find(atoi(dataObject->routeOperator.c_str()));
		dataObject->routeOperator = it != stringTable.end() ? it->second : "";
	}
	if (dataObject->color.size() > 0) {
		const auto it = stringTable.find(atoi(dataObject->color.c_str()));
		dataObject->color = it != stringTable.end() ? it->second : "";
	}
	if (dataObject->type.size() > 0) {
		const auto it = stringTable.find(atoi(dataObject->type.c_str()));
		dataObject->type = it != stringTable.end() ? it->second : "";
	}
	if (!onlyDescription) {
		for (SHARED_PTR<TransportStop>& s : dataObject->forwardStops) {
			initializeNames(stringTable, s);
		}
	}
}

void searchTransportIndex(TransportIndex* index, SearchQuery* q, CodedInputStream* input) {
	if (index->stopsFileLength == 0 || index->right < q->left || index->left > q->right || index->top > q->bottom ||
		index->bottom < q->top) {
		return;
	}
	input->Seek(index->stopsFileOffset);
	int oldLimit = input->PushLimit(index->stopsFileLength);
	uint32_t offset = q->transportResults.size();
	UNORDERED(map)<int32_t, string> stringTable;
	searchTransportTreeBounds(input, 0, 0, 0, 0, q, stringTable);
	input->PopLimit(oldLimit);
	initializeStringTable(input, index, stringTable);
	UNORDERED(map)<int32_t, string> indexedStringTable = index->stringTable->stringTable;
	for (uint64_t i = offset; i < q->transportResults.size(); i++) {
		initializeNames(indexedStringTable, q->transportResults[i]);
	}
	return;
}

void searchTransportIndex(SearchQuery* q, BinaryMapFile* file) {
	lseek(file->routefd, 0, SEEK_SET);
	FileInputStream input(file->routefd);
	input.SetCloseOnDelete(false);
	CodedInputStream cis(&input);
	cis.SetTotalBytesLimit(INT_MAX, INT_MAX >> 1);
	std::vector<TransportIndex*>::iterator transportIndex = file->transportIndexes.begin();
	for (; transportIndex != file->transportIndexes.end(); transportIndex++) {
		searchTransportIndex(*transportIndex, q, &cis);
	}
	if (q->numberOfVisitedObjects > 0) {
		// OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug,  "Search is done. Visit %d objects. Read %d objects.",
		// q->numberOfVisitedObjects, q->numberOfAcceptedObjects); OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug,
		// "Read %d subtrees. Go through %d subtrees. ", q->numberOfReadSubtrees, q->numberOfAcceptedSubtrees);
	}
	return;
}

bool getTransportIndex(int64_t filePointer, TransportIndex*& ind) {
	for (TransportIndex* i : transportIndexesList) {
		if (i->filePointer <= filePointer && (filePointer - i->filePointer) < i->length) {
			ind = i;
			return true;
		}
	}
	return false;
}

void loadTransportRoutes(BinaryMapFile* file, vector<int32_t> filePointers, UNORDERED(map) < int64_t,
						 SHARED_PTR<TransportRoute> > &result) {
	lseek(file->routefd, 0, SEEK_SET);
	FileInputStream input(file->routefd);
	input.SetCloseOnDelete(false);
	CodedInputStream cis(&input);
	cis.SetTotalBytesLimit(INT_MAX, INT_MAX >> 1);

	UNORDERED(map)<TransportIndex*, vector<int32_t>> groupPoints;
	for (int i = 0; i < filePointers.size(); i++) {
		TransportIndex* ind = NULL;
		if (getTransportIndex(filePointers[i], ind)) {
			if (groupPoints.find(ind) == groupPoints.end()) {
				groupPoints[ind] = vector<int32_t>();
			}
			groupPoints[ind].push_back(filePointers[i]);
		}
	}
	auto it = groupPoints.begin();
	while (it != groupPoints.end()) {
		TransportIndex* ind = it->first;
		vector<int32_t> pointers = it->second;
		sort(pointers.begin(), pointers.end());
		UNORDERED(map)<int32_t, string> stringTable;
		vector<SHARED_PTR<TransportRoute>> finishInit;

		for (int i = 0; i < pointers.size(); i++) {
			int32_t filePointer = pointers[i];
			SHARED_PTR<TransportRoute> transportRoute = make_shared<TransportRoute>();
			if (readTransportRoute(file, transportRoute, filePointer, stringTable, false)) {
				result[filePointer] = transportRoute;
				finishInit.push_back(transportRoute);
			}
		}
		initializeStringTable(&cis, ind, stringTable);
		UNORDERED(map)<int32_t, string> indexedStringTable = ind->stringTable->stringTable;
		for (SHARED_PTR<TransportRoute>& transportRoute : finishInit) {
			initializeNames(false, transportRoute, indexedStringTable);
		}
		it++;
	}
}
//----------------------------

bool searchMapTreeBounds(CodedInputStream* input, MapTreeBounds* current, MapTreeBounds* parent, SearchQuery* req,
						 std::vector<MapTreeBounds>* foundSubtrees) {
	int init = 0;
	int tag;
	int si;
	req->numberOfReadSubtrees++;
	int oceanTag = -1;
	while ((tag = input->ReadTag()) != 0) {
		if (req->publisher->isCancelled()) {
			return false;
		}
		if (init == 0xf) {
			init = 0;
			// coordinates are init
			if (current->right < (uint)req->left || current->left > (uint)req->right ||
				current->top > (uint)req->bottom || current->bottom < (uint)req->top) {
				return false;
			} else {
				req->numberOfAcceptedSubtrees++;
			}
		}
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			// required uint32_t version = 1;
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kLeftFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				current->left = si + parent->left;
				init |= 1;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kRightFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				current->right = si + parent->right;
				init |= 2;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kTopFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				current->top = si + parent->top;
				init |= 4;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kBottomFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int32_t, WireFormatLite::TYPE_SINT32>(input, &si)));
				current->bottom = si + parent->bottom;
				init |= 8;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber: {
				readInt(input, &current->mapDataBlock);
				current->mapDataBlock += current->filePointer;
				foundSubtrees->push_back(*current);
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kOceanFieldNumber: {
				bool ocean = false;
				DO_((WireFormatLite::ReadPrimitive<bool, WireFormatLite::TYPE_BOOL>(input, &ocean)));
				oceanTag = ocean ? 1 : 0;
				break;
			}
			case OsmAnd::OBF::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber: {
				MapTreeBounds* child = new MapTreeBounds();
				// ocean set only if there is no children
				oceanTag = -1;
				readInt(input, &child->length);
				child->filePointer = input->TotalBytesRead();
				int oldLimit = input->PushLimit(child->length);
				searchMapTreeBounds(input, child, current, req, foundSubtrees);
				input->PopLimit(oldLimit);
				input->Seek(child->filePointer + child->length);
				delete child;
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	if (oceanTag >= 0) {
		if (current->right < (uint)req->oceanLeft || current->left > (uint)req->oceanRight ||
			current->top > (uint)req->oceanBottom || current->bottom < (uint)req->oceanTop) {
			// OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug,  "Ocean tile = %d %d %d zoom=12",
			//		oceanTag, current->left >> 19, current->top >> 19);
		} else {
			req->oceanTiles++;
			if (oceanTag == 1) {
				req->ocean++;
			}
		}
	}
	return true;
}

bool readMapDataBlocks(CodedInputStream* input, SearchQuery* req, MapTreeBounds* tree, MapIndex* root) {
	uint64_t baseId = 0;
	int tag;
	std::vector<MapDataObject*> results;
	while ((tag = input->ReadTag()) != 0) {
		if (req->publisher->isCancelled()) {
			return false;
		}
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			// required uint32_t version = 1;
			case OsmAnd::OBF::MapDataBlock::kBaseIdFieldNumber: {
				WireFormatLite::ReadPrimitive<uint64_t, WireFormatLite::TYPE_UINT64>(input, &baseId);
				break;
			}
			case OsmAnd::OBF::MapDataBlock::kStringTableFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				if (results.size() > 0) {
					std::vector<std::string> stringTable;
					readStringTable(input, stringTable);
					for (std::vector<MapDataObject*>::iterator obj = results.begin(); obj != results.end(); obj++) {
						if ((*obj)->stringIds.size() > 0) {
							UNORDERED(map)<std::string, unsigned int>::iterator val = (*obj)->stringIds.begin();
							while (val != (*obj)->stringIds.end()) {
								(*obj)->objectNames[val->first] = stringTable[val->second];
								val++;
							}
						}
					}
				}
				input->Skip(input->BytesUntilLimit());
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::MapDataBlock::kDataObjectsFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				MapDataObject* mapObject = readMapDataObject(input, tree, req, root, baseId);
				if (mapObject != NULL) {
					mapObject->id += baseId;
					if (req->publish(mapObject, root, req->zoom)) {
						results.push_back(mapObject);
					} else {
						delete mapObject;
					}
				}
				input->Skip(input->BytesUntilLimit());
				input->PopLimit(oldLimit);
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

bool checkObjectBounds(SearchQuery* q, MapDataObject* o) {
	uint prevCross = 0;
	for (uint i = 0; i < o->points.size(); i++) {
		uint cross = 0;
		int x31 = o->points[i].first;
		int y31 = o->points[i].second;
		cross |= (x31 < q->left ? 1 : 0);
		cross |= (x31 > q->right ? 2 : 0);
		cross |= (y31 < q->top ? 4 : 0);
		cross |= (y31 > q->bottom ? 8 : 0);
		if (i > 0) {
			if ((prevCross & cross) == 0) {
				return true;
			}
		}
		prevCross = cross;
	}
	return false;
}

bool sortTreeBounds(const MapTreeBounds& i, const MapTreeBounds& j) {
	return (i.mapDataBlock < j.mapDataBlock);
}

void searchMapData(CodedInputStream* input, MapRoot* root, MapIndex* ind, SearchQuery* req) {
	// search
	for (std::vector<MapTreeBounds>::iterator i = root->bounds.begin(); i != root->bounds.end(); i++) {
		if (req->publisher->isCancelled()) {
			return;
		}
		if (i->right < (uint)req->left || i->left > (uint)req->right || i->top > (uint)req->bottom ||
			i->bottom < (uint)req->top) {
			continue;
		}
		std::vector<MapTreeBounds> foundSubtrees;
		input->Seek(i->filePointer);
		int oldLimit = input->PushLimit(i->length);
		searchMapTreeBounds(input, &(*i), root, req, &foundSubtrees);
		input->PopLimit(oldLimit);

		sort(foundSubtrees.begin(), foundSubtrees.end(), sortTreeBounds);
		uint32_t length;
		for (std::vector<MapTreeBounds>::iterator tree = foundSubtrees.begin(); tree != foundSubtrees.end(); tree++) {
			if (req->publisher->isCancelled()) {
				return;
			}
			input->Seek(tree->mapDataBlock);
			WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length);
			int oldLimit = input->PushLimit(length);
			readMapDataBlocks(input, req, &(*tree), ind);
			input->PopLimit(oldLimit);
		}
	}
}

void convertRouteDataObjecToMapObjects(SearchQuery* q, std::vector<RouteDataObject*>& list,
									   std::vector<FoundMapDataObject>& tempResult, int& renderedState) {
	std::vector<RouteDataObject*>::iterator rIterator = list.begin();
	tempResult.reserve((size_t)(list.size() + tempResult.size()));
	for (; rIterator != list.end(); rIterator++) {
		RouteDataObject* r = (*rIterator);
		if (r == NULL) {
			continue;
		}
		// if (skipDuplicates && r->id > 0) {
		// 	if (ids.find(r->id) != ids.end()) {
		// 		continue;
		// 	}
		// 	ids.insert(r->id);
		// }
		MapDataObject* obj = new MapDataObject;
		bool add = true;
		std::vector<uint32_t>::iterator typeIt = r->types.begin();
		for (; typeIt != r->types.end(); typeIt++) {
			uint32_t k = (*typeIt);
			if (k < r->region->routeEncodingRules.size()) {
				auto& t = r->region->routeEncodingRules[k];
				if (t.getTag() == "highway" || t.getTag() == "route" || t.getTag() == "railway" ||
					t.getTag() == "aeroway" || t.getTag() == "aerialway") {
					obj->types.push_back(tag_value(t.getTag(), t.getValue()));
				} else {
					obj->additionalTypes.push_back(tag_value(t.getTag(), t.getValue()));
				}
			}
		}
		if (add) {
			for (uint32_t s = 0; s < r->pointsX.size(); s++) {
				obj->points.push_back(std::pair<int, int>(r->pointsX[s], r->pointsY[s]));
			}
			obj->id = r->id;
			UNORDERED(map)<int, std::string>::iterator nameIterator = r->names.begin();
			for (; nameIterator != r->names.end(); nameIterator++) {
				std::string ruleId = r->region->routeEncodingRules[nameIterator->first].getTag();
				obj->objectNames[ruleId] = nameIterator->second;
			}

			for (auto nIterator = r->namesIds.begin(); nIterator != r->namesIds.end(); nIterator++) {
				obj->namesOrder.push_back(r->region->routeEncodingRules[nIterator->first].getTag());
			}

			obj->area = false;
			if (renderedState < 2 && checkObjectBounds(q, obj)) {
				renderedState |= 2;
			}
			tempResult.push_back(FoundMapDataObject(obj, NULL, q->zoom));
		} else {
			delete obj;
		}
		delete r;
	}
}

void checkAndInitRouteRegionRules(int fileInd, RoutingIndex* routingIndex) {
	// init decoding rules
	if (routingIndex->routeEncodingRules.size() == 0) {
		lseek(fileInd, 0, SEEK_SET);
		FileInputStream input(fileInd);
		input.SetCloseOnDelete(false);
		CodedInputStream cis(&input);
		cis.SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);

		cis.Seek(routingIndex->filePointer);
		uint32_t old = cis.PushLimit(routingIndex->length);
		readRoutingIndex(&cis, routingIndex, true);
		cis.PopLimit(old);
	}
}

void searchRouteSubregions(SearchQuery* q, std::vector<RouteSubregion>& tempResult, bool basemap, bool geocoding) {
	vector<BinaryMapFile*>::iterator i = openFiles.begin();
	for (; i != openFiles.end() && !q->publisher->isCancelled(); i++) {
		BinaryMapFile* file = *i;
		std::vector<RoutingIndex*>::iterator routeIndex = file->routingIndexes.begin();
		for (; routeIndex != file->routingIndexes.end(); routeIndex++) {
			bool contains = false;
			std::vector<RouteSubregion>& subs = basemap ? (*routeIndex)->basesubregions : (*routeIndex)->subregions;
			for (std::vector<RouteSubregion>::iterator subreg = subs.begin(); subreg != subs.end(); subreg++) {
				if (subreg->right >= (uint)q->left && (uint)q->right >= subreg->left &&
					subreg->bottom >= (uint)q->top && (uint)q->bottom >= subreg->top) {
					contains = true;
				}
			}
			if (contains) {
				FileInputStream* nt = NULL;
				CodedInputStream* cis = NULL;
				searchRouteRegion(&cis, &nt, file, q, *routeIndex, subs, tempResult, geocoding);
				if (cis != NULL) {
					delete cis;
				}
				if (nt != NULL) {
					delete nt;
				}
				checkAndInitRouteRegionRules(geocoding ? file->geocodingfd : file->routefd, (*routeIndex));
			}
		}
	}
}

void readRouteMapObjects(SearchQuery* q, BinaryMapFile* file, vector<RouteSubregion>& found, RoutingIndex* routeIndex,
						 std::vector<FoundMapDataObject>& tempResult, int& renderedState) {
	sort(found.begin(), found.end(), sortRouteRegions);
	lseek(file->fd, 0, SEEK_SET);
	FileInputStream input(file->fd);
	input.SetCloseOnDelete(false);
	CodedInputStream cis(&input);
	cis.SetTotalBytesLimit(INT_MAX, INT_MAX >> 2);
	for (std::vector<RouteSubregion>::iterator sub = found.begin(); sub != found.end(); sub++) {
		std::vector<RouteDataObject*> list;
		cis.Seek(sub->filePointer + sub->mapDataBlock);
		uint32_t length;
		cis.ReadVarint32(&length);
		uint32_t old = cis.PushLimit(length);
		readRouteTreeData(&cis, &(*sub), list, routeIndex);
		cis.PopLimit(old);
		convertRouteDataObjecToMapObjects(q, list, tempResult, renderedState);
	}
}

void readRouteDataAsMapObjects(SearchQuery* q, BinaryMapFile* file, std::vector<FoundMapDataObject>& tempResult,
							   int& renderedState) {
	std::vector<RoutingIndex*>::iterator routeIndex = file->routingIndexes.begin();
	for (; routeIndex != file->routingIndexes.end(); routeIndex++) {
		if (q->publisher->isCancelled()) {
			break;
		}
		bool contains = false;
		std::vector<RouteSubregion> subs = (*routeIndex)->subregions;
		if (q->zoom <= zoomForBaseRouteRendering) {
			subs = (*routeIndex)->basesubregions;
		}
		for (std::vector<RouteSubregion>::iterator subreg = subs.begin(); subreg != subs.end(); subreg++) {
			if (subreg->right >= (uint)q->left && (uint)q->right >= subreg->left && subreg->bottom >= (uint)q->top &&
				(uint)q->bottom >= subreg->top) {
				OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Search route map %s", (*routeIndex)->name.c_str());
				contains = true;
			}
		}
		if (contains) {
			vector<RouteSubregion> found;
			FileInputStream* nt = NULL;
			CodedInputStream* cis = NULL;
			searchRouteRegion(&cis, &nt, file, q, *routeIndex, subs, found, false);
			if (cis != NULL) {
				delete cis;
			}
			if (nt != NULL) {
				delete nt;
			}
			checkAndInitRouteRegionRules(file->fd, (*routeIndex));
			readRouteMapObjects(q, file, found, (*routeIndex), tempResult, renderedState);
		}
	}
}

void readMapObjects(SearchQuery* q, BinaryMapFile* file) {
	for (std::vector<MapIndex>::iterator mapIndex = file->mapIndexes.begin(); mapIndex != file->mapIndexes.end();
		 mapIndex++) {
		for (std::vector<MapRoot>::iterator mapLevel = mapIndex->levels.begin(); mapLevel != mapIndex->levels.end();
			 mapLevel++) {
			if (q->publisher->isCancelled()) {
				break;
			}

			if (mapLevel->minZoom <= q->zoom && mapLevel->maxZoom >= q->zoom) {
				if (mapLevel->right >= (uint)q->left && (uint)q->right >= mapLevel->left &&
					mapLevel->bottom >= (uint)q->top && (uint)q->bottom >= mapLevel->top) {
					// OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Search map %s", mapIndex->name.c_str());
					// lazy initializing rules
					if (mapIndex->decodingRules.size() == 0) {
						lseek(file->fd, 0, SEEK_SET);
						FileInputStream input(file->fd);
						input.SetCloseOnDelete(false);
						CodedInputStream cis(&input);
						cis.SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);
						cis.Seek(mapIndex->filePointer);
						int oldLimit = cis.PushLimit(mapIndex->length);
						readMapIndex(&cis, &(*mapIndex), true);
						cis.PopLimit(oldLimit);
					}
					// lazy initializing subtrees
					if (mapLevel->bounds.size() == 0) {
						lseek(file->fd, 0, SEEK_SET);
						FileInputStream input(file->fd);
						input.SetCloseOnDelete(false);
						CodedInputStream cis(&input);
						cis.SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);
						cis.Seek(mapLevel->filePointer);
						int oldLimit = cis.PushLimit(mapLevel->length);
						readMapLevel(&cis, &(*mapLevel), true);
						cis.PopLimit(oldLimit);
					}
					lseek(file->fd, 0, SEEK_SET);
					FileInputStream input(file->fd);
					input.SetCloseOnDelete(false);
					CodedInputStream cis(&input);
					cis.SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);
					searchMapData(&cis, &(*mapLevel), &(*mapIndex), q);
				}
			}
		}
	}
}

void readMapObjectsForRendering(SearchQuery* q, std::vector<FoundMapDataObject>& basemapResult,
								std::vector<FoundMapDataObject>& tempResult, std::vector<FoundMapDataObject>& extResult,
								std::vector<FoundMapDataObject>& coastLines,
								std::vector<FoundMapDataObject>& basemapCoastLines, int& count, bool& basemapExists,
								int& renderedState) {
	vector<BinaryMapFile*>::iterator i = openFiles.begin();
	for (; i != openFiles.end() && !q->publisher->isCancelled(); i++) {
		BinaryMapFile* file = *i;
		basemapExists |= file->isBasemap();
	}
	i = openFiles.begin();
	int oleft, sleft, bleft;
	int otop, stop, btop;
	int oright, sright, bright;
	int obottom, sbottom, bbottom;
	q->oceanLeft = sleft = oleft = bleft = q->left;
	q->oceanBottom = sbottom = obottom = bbottom = q->bottom;
	q->oceanTop = stop = otop = btop = q->top;
	q->oceanRight = sright = oright = bright = q->right;
	if (q->zoom > zoomOnlyForBasemaps) {
		// expand area to include 11-zoom coastlines for bbox
		int shift = (31 - zoomOnlyForBasemaps);
		bleft = (q->left >> shift) << shift;
		bright = ((q->right >> shift) + 1) << shift;
		btop = (q->top >> shift) << shift;
		bbottom = ((q->bottom >> shift) + 1) << shift;
	}
	if (q->zoom > zoomDetailedForCoastlines + 1) {
		// expand area to include more coastlines for bbox
		int shift = (31 - zoomDetailedForCoastlines + 1);
		sleft = (q->left >> shift) << shift;
		sright = ((q->right >> shift) + 1) << shift;
		stop = (q->top >> shift) << shift;
		sbottom = ((q->bottom >> shift) + 1) << shift;
	}
	UNORDERED(set)<uint64_t> deletedIds;
	for (; i != openFiles.end() && !q->publisher->isCancelled(); i++) {
		BinaryMapFile* file = *i;
		if (q->req != NULL) {
			q->req->clearState();
		}
		q->publisher->clear();
		if (!q->publisher->isCancelled()) {
			bool basemap = file->isBasemap();
			bool external = file->isExternal();

			if (basemap) {
				q->left = bleft;
				q->right = bright;
				q->top = btop;
				q->bottom = bbottom;
			} else {
				q->left = sleft;
				q->right = sright;
				q->top = stop;
				q->bottom = sbottom;
			}
			readMapObjects(q, file);
			std::vector<FoundMapDataObject>::iterator r = q->publisher->result.begin();
			tempResult.reserve((size_t)(q->publisher->result.size() + tempResult.size()));

			for (; r != q->publisher->result.end(); r++) {
				if (basemap) {
					if (renderedState % 2 == 0 && checkObjectBounds(q, r->obj)) {
						renderedState |= 1;
					}
				} else {
					if (renderedState < 2 && checkObjectBounds(q, r->obj)) {
						renderedState |= 2;
					}
				}

				count++;
				if (!basemap && r->obj->contains("osmand_change", "delete")) {
					deletedIds.insert(r->obj->id);
				}
				if (r->obj->contains("natural", "coastline")) {
					//&& !(*r)->contains("place", "island")
					if (basemap) {
						basemapCoastLines.push_back(*r);
					} else {
						if (deletedIds.find(r->obj->id) == deletedIds.end()) {
							coastLines.push_back(*r);
						}
					}
				} else {
					// do not mess coastline and other types
					if (basemap) {
						basemapResult.push_back(*r);
					} else if (external) {
						extResult.push_back(*r);
					} else {
						tempResult.push_back(*r);
						// renderRouteDataFile = -1;
					}
				}
			}
			q->publisher->clear();
		}
	}
	q->left = oleft;
	q->right = oright;
	q->top = otop;
	q->bottom = obottom;
}

std::string simpleNonLiveName(std::string s) {
	uint t = s.length() - 1;
	for (; t > 0; t--) {
		if (s[t] == '_' || (s[t] >= '0' && s[t] < '9')) {
			// strip numbers
		} else {
			break;
		}
	}
	return t > 0 ? s.substr(0, t + 1) : s;
}

bool ResultPublisher::publish(FoundMapDataObject o) {
	MapDataObject* r = o.obj;
	// MapIndex *ind = (MapIndex *) o.ind;
	if (r->id > 0) {
		auto it = ids.find(r->id);
		if (it != ids.end()) {
			MapDataObject* ex = it->second.obj;
			// MapIndex *exInd = (MapIndex *) it->second.ind;
			// // check that object is not from newer live maps
			// bool newerLiveMap = false;
			// if (ind != NULL && exInd != NULL) {
			// 	if (simpleNonLiveName(exInd->name) == simpleNonLiveName(ind->name)) {
			// 		newerLiveMap = true;
			// 	}
			// }
			if (ex->points.size() >= r->points.size() || o.zoom >= 13) {
				return false;
			} else {
				auto it = result.begin();
				for (; it != result.end(); it++) {
					if (it->obj == ex) {
						result.erase(it);
						break;
					}
				}
				delete ex;
			}
		}
		ids[r->id] = o;
	}
	result.push_back(o);
	return true;
}

void uniq(std::vector<FoundMapDataObject>& r, std::vector<FoundMapDataObject>& uniq) {
	UNORDERED(set)<uint64_t> ids;
	for (uint i = 0; i < r.size(); i++) {
		if (r[i].obj->id > 0 && !ids.insert(r[i].obj->id).second) {
			continue;
		} else {
			uniq.push_back(r[i]);
		}
	}
}

ResultPublisher* searchObjectsForRendering(SearchQuery* q, bool skipDuplicates, std::string msgNothingFound,
										   int& renderedState) {
	int count = 0;
	std::vector<FoundMapDataObject> basemapResult;
	std::vector<FoundMapDataObject> tempResult;
	std::vector<FoundMapDataObject> extResult;
	std::vector<FoundMapDataObject> coastLines;
	std::vector<FoundMapDataObject> uniqCoastLines;
	std::vector<FoundMapDataObject> basemapCoastLines;

	bool basemapExists = false;
	readMapObjectsForRendering(q, basemapResult, tempResult, extResult, coastLines, basemapCoastLines, count,
							   basemapExists, renderedState);

	// bool objectsFromMapSectionRead = tempResult.size() > 0;
	bool objectsFromRoutingSectionRead = false;
	if (q->zoom >= zoomOnlyForBasemaps) {
		vector<BinaryMapFile*>::iterator i = openFiles.begin();
		for (; i != openFiles.end() && !q->publisher->isCancelled(); i++) {
			BinaryMapFile* file = *i;
			// false positive case when we have 2 sep maps Country-roads & Country
			if (file->isRoadOnly()) {
				if (q->req != NULL) {
					q->req->clearState();
				}
				q->publisher->clear();
				auto sz = tempResult.size();
				readRouteDataAsMapObjects(q, file, tempResult, renderedState);
				objectsFromRoutingSectionRead = tempResult.size() != sz;
			}
		}
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Route objects %d", tempResult.size());
	}

	// sort results/ analyze coastlines and publish back to publisher
	if (q->publisher->isCancelled()) {
		deleteObjects(coastLines);
		deleteObjects(tempResult);
		deleteObjects(basemapCoastLines);
		deleteObjects(basemapResult);
	} else {
		float ocean = q->oceanTiles > 0 ? ((float)q->ocean) / q->oceanTiles : 0;
		bool addBasemapCoastlines = true;
		bool emptyData = q->zoom > zoomOnlyForBasemaps && tempResult.empty() && coastLines.empty();
		// determine if there are enough objects like land/lake..
		bool basemapMissing = q->zoom <= zoomOnlyForBasemaps && basemapCoastLines.empty() && !basemapExists;
		// bool detailedLandData = q->zoom >= 14 && tempResult.size() > 0 && objectsFromMapSectionRead;
		bool coastlinesWereAdded = false;
		bool detailedCoastlinesWereAdded = false;
		if (!coastLines.empty() && q->zoom > zoomOnlyForBasemaps) {
			int bleft = q->left;
			int bright = q->right;
			int btop = q->top;
			int bbottom = q->bottom;
			if (q->zoom > zoomDetailedForCoastlines + 1) {
				int shift = (31 - zoomDetailedForCoastlines + 1);
				bleft = (q->left >> shift) << shift;
				bright = ((q->right >> shift) + 1) << shift;
				btop = (q->top >> shift) << shift;
				bbottom = ((q->bottom >> shift) + 1) << shift;
			}
			uniq(coastLines, uniqCoastLines);
			coastlinesWereAdded = processCoastlines(uniqCoastLines, bleft, bright, bbottom, btop, q->zoom,
													basemapCoastLines.empty(), true, tempResult);
			// addBasemapCoastlines = (!coastlinesWereAdded && !detailedLandData) || q->zoom <= zoomOnlyForBasemaps;
			addBasemapCoastlines = !coastlinesWereAdded;
		} else {
			// addBasemapCoastlines = !detailedLandData;
			addBasemapCoastlines = true;
		}
		detailedCoastlinesWereAdded = coastlinesWereAdded;

		if (addBasemapCoastlines) {
			int bleft = q->left;
			int bright = q->right;
			int btop = q->top;
			int bbottom = q->bottom;
			if (q->zoom > zoomOnlyForBasemaps) {
				int shift = (31 - zoomOnlyForBasemaps);
				bleft = (q->left >> shift) << shift;
				bright = ((q->right >> shift) + 1) << shift;
				btop = (q->top >> shift) << shift;
				bbottom = ((q->bottom >> shift) + 1) << shift;
			}
			coastlinesWereAdded =
				processCoastlines(basemapCoastLines, bleft, bright, bbottom, btop, q->zoom, true, true, tempResult);
		}
		// processCoastlines always create new objects
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,
						  "Detailed coastlines = %d, basemap coastlines %d, ocean tile %f. Detailed added %d, basemap "
						  "processed %d, basemap added %d.",
						  coastLines.size(), basemapCoastLines.size(), ocean, detailedCoastlinesWereAdded,
						  addBasemapCoastlines, (addBasemapCoastlines ? coastlinesWereAdded : false));
		deleteObjects(basemapCoastLines);
		deleteObjects(coastLines);
		if (!coastlinesWereAdded && ocean > 0.5) {
			MapDataObject* o = new MapDataObject();
			o->points.push_back(int_pair(q->left, q->top));
			o->points.push_back(int_pair(q->right, q->top));
			o->points.push_back(int_pair(q->right, q->bottom));
			o->points.push_back(int_pair(q->left, q->bottom));
			o->points.push_back(int_pair(q->left, q->top));
			if (ocean) {
				o->types.push_back(tag_value("natural", "coastline"));
			} else {
				o->types.push_back(tag_value("natural", "land"));
			}
			o->area = true;
			o->additionalTypes.push_back(tag_value("layer", "-5"));
			tempResult.push_back(FoundMapDataObject(o, NULL, q->zoom));
		}
		if ((emptyData && extResult.size() == 0) || basemapMissing) {
			// message
			// avoid overflow int errors
			MapDataObject* o = new MapDataObject();
			o->points.push_back(int_pair(q->left + (q->right - q->left) / 2, q->top + (q->bottom - q->top) / 2));
			o->types.push_back(tag_value("natural", "coastline"));
			o->objectNames["name"] = msgNothingFound;
			o->namesOrder.push_back("name");
			tempResult.push_back(FoundMapDataObject(o, NULL, q->zoom));
		}
		if (q->zoom <= zoomOnlyForBasemaps || emptyData ||
			(objectsFromRoutingSectionRead && q->zoom < detailedZoomStartForRouteSection)) {
			tempResult.insert(tempResult.end(), basemapResult.begin(), basemapResult.end());
		} else {
			deleteObjects(basemapResult);
		}
		q->publisher->clear();
		q->publisher->publishOnlyUnique(tempResult);
		q->publisher->publishAll(extResult);
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info,
						  "Search : tree - read( %d), accept( %d), objs - visit( %d), accept(%d), in result(%d) ",
						  q->numberOfReadSubtrees, q->numberOfAcceptedSubtrees, q->numberOfVisitedObjects,
						  q->numberOfAcceptedObjects, q->publisher->result.size());
	}
	return q->publisher;
}

void initInputForRouteFile(CodedInputStream** inputStream, FileInputStream** fis, BinaryMapFile* file, uint32_t seek,
						   bool geocoding) {
	if (*inputStream == 0) {
		lseek(geocoding ? file->geocodingfd : file->routefd, 0, SEEK_SET);	// seek 0 or seek (*routeIndex)->filePointer
		*fis = new FileInputStream(geocoding ? file->geocodingfd : file->routefd);
		(*fis)->SetCloseOnDelete(false);
		*inputStream = new CodedInputStream(*fis);
		(*inputStream)->SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);
		(*inputStream)->PushLimit(INT_MAXIMUM);
		// inputStream -> Seek((*routeIndex)->filePointer);
		(*inputStream)->Seek(seek);
	} else {
		(*inputStream)->Seek(seek);
	}
}

void searchRouteRegion(CodedInputStream** input, FileInputStream** fis, BinaryMapFile* file, SearchQuery* q,
					   RoutingIndex* ind, std::vector<RouteSubregion>& subregions, std::vector<RouteSubregion>& toLoad,
					   bool geocoding) {
	for (std::vector<RouteSubregion>::iterator subreg = subregions.begin(); subreg != subregions.end(); subreg++) {
		if (subreg->right >= (uint)q->left && (uint)q->right >= subreg->left && subreg->bottom >= (uint)q->top &&
			(uint)q->bottom >= subreg->top) {
			if (subreg->subregions.empty() && subreg->mapDataBlock == 0) {
				initInputForRouteFile(input, fis, file, subreg->filePointer, geocoding);
				uint32_t old = (*input)->PushLimit(subreg->length);
				readRouteTree(*input, &(*subreg), NULL, ind, -1 /*contains? -1 : 1*/, false);
				(*input)->PopLimit(old);
			}
			searchRouteRegion(input, fis, file, q, ind, subreg->subregions, toLoad, geocoding);
			if (subreg->mapDataBlock != 0) {
				toLoad.push_back(*subreg);
			}
		}
	}
}

bool readRouteDataObject(CodedInputStream* input, uint32_t left, uint32_t top, RouteDataObject* obj) {
	int tag;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			case OsmAnd::OBF::RouteData::kTypesFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				uint32_t t;
				while (input->BytesUntilLimit() > 0) {
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &t)));
					obj->types.push_back(t);
				}
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::RouteData::kRouteIdFieldNumber: {
				DO_((WireFormatLite::ReadPrimitive<int64_t, WireFormatLite::TYPE_INT64>(input, &obj->id)));
				break;
			}
			case OsmAnd::OBF::RouteData::kPointsFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				int s;
				int px = left >> ROUTE_SHIFT_COORDINATES;
				int py = top >> ROUTE_SHIFT_COORDINATES;
				while (input->BytesUntilLimit() > 0) {
					DO_((WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_SINT32>(input, &s)));
					uint32_t x = s + px;
					DO_((WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_SINT32>(input, &s)));
					uint32_t y = s + py;

					obj->pointsX.push_back(x << ROUTE_SHIFT_COORDINATES);
					obj->pointsY.push_back(y << ROUTE_SHIFT_COORDINATES);
					px = x;
					py = y;
				}
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::RouteData::kStringNamesFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				uint32_t s;
				uint32_t t;
				while (input->BytesUntilLimit() > 0) {
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &s)));
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &t)));

					obj->namesIds.push_back(pair<uint32_t, uint32_t>(s, t));
				}
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::RouteData::kPointNamesFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				while (input->BytesUntilLimit() > 0) {
					uint32_t pointInd;
					uint32_t nameType;
					uint32_t name;
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &pointInd)));
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &nameType)));
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &name)));
					if (obj->pointNameTypes.size() <= pointInd) {
						obj->pointNameTypes.resize(pointInd + 1, std::vector<uint32_t>());
					}
					obj->pointNameTypes[pointInd].push_back(nameType);
					if (obj->pointNameIds.size() <= pointInd) {
						obj->pointNameIds.resize(pointInd + 1, std::vector<uint32_t>());
					}
					obj->pointNameIds[pointInd].push_back(name);
				}
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::RouteData::kPointTypesFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				while (input->BytesUntilLimit() > 0) {
					uint32_t pointInd;
					uint32_t lens;
					uint32_t t;
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &pointInd)));
					DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &lens)));
					int oldLimits = input->PushLimit(lens);

					if (obj->pointTypes.size() <= pointInd) {
						obj->pointTypes.resize(pointInd + 1, std::vector<uint32_t>());
					}
					while (input->BytesUntilLimit() > 0) {
						DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &t)));
						obj->pointTypes[pointInd].push_back(t);
					}
					input->PopLimit(oldLimits);
				}
				input->PopLimit(oldLimit);
				break;
			}

			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	return true;
}

bool readRouteTreeData(CodedInputStream* input, RouteSubregion* s, std::vector<RouteDataObject*>& dataObjects,
					   RoutingIndex* routingIndex) {
	int tag;
	std::vector<int64_t> idTables;
	UNORDERED(map)<int64_t, std::vector<RestrictionInfo>> restrictions;
	std::vector<std::string> stringTable;
	while ((tag = input->ReadTag()) != 0) {
		switch (WireFormatLite::GetTagFieldNumber(tag)) {
			// required uint32_t version = 1;
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBlock::kDataObjectsFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				RouteDataObject* obj = new RouteDataObject;
				readRouteDataObject(input, s->left, s->top, obj);
				if ((uint32_t)dataObjects.size() <= obj->id) {
					dataObjects.resize((uint32_t)obj->id + 1, NULL);
				}
				obj->region = routingIndex;
				dataObjects[obj->id] = obj;
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBlock::kStringTableFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				readStringTable(input, stringTable);
				input->Skip(input->BytesUntilLimit());
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBlock::kRestrictionsFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				uint64_t from;
				RestrictionInfo info;
				int tm;
				int ts;
				while ((ts = input->ReadTag()) != 0) {
					switch (WireFormatLite::GetTagFieldNumber(ts)) {
						case OsmAnd::OBF::RestrictionData::kFromFieldNumber: {
							DO_((WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_INT32>(input, &tm)));
							from = tm;
							break;
						}
						case OsmAnd::OBF::RestrictionData::kToFieldNumber: {
							DO_((WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_INT32>(input, &tm)));
							info.to = tm;
							break;
						}
						case OsmAnd::OBF::RestrictionData::kViaFieldNumber: {
							DO_((WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_INT32>(input, &tm)));
							info.via = tm;
							break;
						}
						case OsmAnd::OBF::RestrictionData::kTypeFieldNumber: {
							DO_((WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_INT32>(input, &tm)));
							info.type = tm;
							break;
						}
						default: {
							if (WireFormatLite::GetTagWireType(ts) == WireFormatLite::WIRETYPE_END_GROUP) {
								return true;
							}
							if (!skipUnknownFields(input, ts)) {
								return false;
							}
							break;
						}
					}
				}
				restrictions[from].push_back(info);
				input->PopLimit(oldLimit);
				break;
			}
			case OsmAnd::OBF::OsmAndRoutingIndex_RouteDataBlock::kIdTableFieldNumber: {
				uint32_t length;
				DO_((WireFormatLite::ReadPrimitive<uint32_t, WireFormatLite::TYPE_UINT32>(input, &length)));
				int oldLimit = input->PushLimit(length);
				int64_t routeId = 0;
				int ts;
				while ((ts = input->ReadTag()) != 0) {
					switch (WireFormatLite::GetTagFieldNumber(ts)) {
						case OsmAnd::OBF::IdTable::kRouteIdFieldNumber: {
							int64_t val;
							DO_((WireFormatLite::ReadPrimitive<int64_t, WireFormatLite::TYPE_SINT64>(input, &val)));
							routeId += val;
							idTables.push_back(routeId);
							break;
						}
						default: {
							if (WireFormatLite::GetTagWireType(ts) == WireFormatLite::WIRETYPE_END_GROUP) {
								return true;
							}
							if (!skipUnknownFields(input, ts)) {
								return false;
							}
							break;
						}
					}
				}
				input->PopLimit(oldLimit);
				break;
			}
			default: {
				if (WireFormatLite::GetTagWireType(tag) == WireFormatLite::WIRETYPE_END_GROUP) {
					return true;
				}
				if (!skipUnknownFields(input, tag)) {
					return false;
				}
				break;
			}
		}
	}
	UNORDERED(map)<int64_t, std::vector<RestrictionInfo>>::iterator itRestrictions = restrictions.begin();
	for (; itRestrictions != restrictions.end(); itRestrictions++) {
		RouteDataObject* fromr = dataObjects[itRestrictions->first];
		if (fromr != NULL) {
			fromr->restrictions = itRestrictions->second;
			for (uint i = 0; i < fromr->restrictions.size(); i++) {
				fromr->restrictions[i].to = idTables[fromr->restrictions[i].to];
				if (fromr->restrictions[i].via != 0) {
					fromr->restrictions[i].via = idTables[fromr->restrictions[i].via];
				}
			}
		}
	}
	std::vector<RouteDataObject*>::iterator dobj = dataObjects.begin();
	for (; dobj != dataObjects.end(); dobj++) {
		if (*dobj != NULL) {
			if ((uint)(*dobj)->id < idTables.size()) {
				(*dobj)->id = idTables[(*dobj)->id];
			}
			if ((*dobj)->namesIds.size() > 0) {
				vector<pair<uint32_t, uint32_t>>::iterator itnames = (*dobj)->namesIds.begin();
				for (; itnames != (*dobj)->namesIds.end(); itnames++) {
					if ((*itnames).second >= stringTable.size()) {
						OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "ERROR VALUE string table %d",
										  (*itnames).second);
					} else {
						(*dobj)->names[(int)(*itnames).first] = stringTable[(*itnames).second];
					}
				}
			}
			for (uint k = 0; k < (*dobj)->pointNameIds.size(); k++) {
				std::vector<uint32_t> vec = (*dobj)->pointNameIds[k];
				std::vector<std::string> res;
				for (uint kl = 0; kl < vec.size(); kl++) {
					if (vec[kl] >= stringTable.size()) {
						OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "ERROR VALUE string table %d", vec[kl]);
					} else {
						res.push_back(stringTable[vec[kl]]);
					}
				}
				(*dobj)->pointNames.push_back(res);
			}
		}
	}

	return true;
}

void searchRouteSubRegion(int fileInd, std::vector<RouteDataObject*>& list, RoutingIndex* routingIndex,
						  RouteSubregion* sub) {
	checkAndInitRouteRegionRules(fileInd, routingIndex);

	// could be simplified but it will be concurrency with init block
	lseek(fileInd, 0, SEEK_SET);
	FileInputStream input(fileInd);
	input.SetCloseOnDelete(false);
	CodedInputStream cis(&input);
	cis.SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);

	cis.Seek(sub->filePointer + sub->mapDataBlock);
	uint32_t length;
	cis.ReadVarint32(&length);
	uint32_t old = cis.PushLimit(length);
	readRouteTreeData(&cis, &(*sub), list, routingIndex);
	cis.PopLimit(old);
}

void searchRouteDataForSubRegion(SearchQuery* q, std::vector<RouteDataObject*>& list, RouteSubregion* sub,
								 bool geocoding) {
	vector<BinaryMapFile*>::iterator i = openFiles.begin();
	RoutingIndex* rs = sub->routingIndex;
	for (; i != openFiles.end() && !q->publisher->isCancelled(); i++) {
		BinaryMapFile* file = *i;
		for (std::vector<RoutingIndex*>::iterator routingIndex = file->routingIndexes.begin();
			 routingIndex != file->routingIndexes.end(); routingIndex++) {
			if (q->publisher->isCancelled()) {
				break;
			}
			if (rs != NULL && (rs->name != (*routingIndex)->name || rs->filePointer != (*routingIndex)->filePointer)) {
				continue;
			}
			searchRouteSubRegion(geocoding ? file->geocodingfd : file->routefd, list, (*routingIndex), sub);
			return;
		}
	}
}

bool closeBinaryMapFile(std::string inputName) {
	std::vector<BinaryMapFile*>::iterator iterator = openFiles.begin();
	for (; iterator != openFiles.end(); iterator++) {
		if ((*iterator)->inputName == inputName) {
			delete *iterator;
			openFiles.erase(iterator);
			return true;
		}
	}
	return false;
}

bool initMapFilesFromCache(std::string inputName) {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
#if defined(_WIN32)
	int fileDescriptor = open(inputName.c_str(), O_RDONLY | O_BINARY);
#else
	int fileDescriptor = open(inputName.c_str(), O_RDONLY);
#endif
	if (fileDescriptor < 0) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "Cache file could not be open to read : %s",
						  inputName.c_str());
		return false;
	}
	FileInputStream input(fileDescriptor);
	CodedInputStream cis(&input);
	cis.SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);
	OsmAnd::OBF::OsmAndStoredIndex* c = new OsmAnd::OBF::OsmAndStoredIndex();
	if (c->MergeFromCodedStream(&cis)) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, "Native Cache file initialized %s", inputName.c_str());
		cache = c;
		return true;
	}
	return false;
}

bool hasEnding(std::string const& fullString, std::string const& ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	} else {
		return false;
	}
}

BinaryMapFile* initBinaryMapFile(std::string inputName, bool useLive, bool routingOnly) {
	GOOGLE_PROTOBUF_VERIFY_VERSION;
	std::map<std::string, BinaryMapFile*>::iterator iterator;
	closeBinaryMapFile(inputName);

#if defined(_WIN32)
	int fileDescriptor = open(inputName.c_str(), O_RDONLY | O_BINARY);
	int routeDescriptor = open(inputName.c_str(), O_RDONLY | O_BINARY);
	int geocodingDescriptor = open(inputName.c_str(), O_RDONLY | O_BINARY);
#else
	int fileDescriptor = open(inputName.c_str(), O_RDONLY);
	int routeDescriptor = open(inputName.c_str(), O_RDONLY);
	int geocodingDescriptor = open(inputName.c_str(), O_RDONLY);
#endif
	if (fileDescriptor < 0 || routeDescriptor < 0 || routeDescriptor == fileDescriptor) {
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "File could not be open to read from C : %s",
						  inputName.c_str());
		return NULL;
	}
	BinaryMapFile* mapFile = new BinaryMapFile();
	mapFile->fd = fileDescriptor;

	mapFile->routefd = routeDescriptor;
	mapFile->geocodingfd = geocodingDescriptor;
	mapFile->liveMap = inputName.find("live/") != string::npos;
	mapFile->inputName = inputName;
	mapFile->roadOnly = inputName.find(".road") != string::npos;
	OsmAnd::OBF::FileIndex* fo = NULL;
	if (cache != NULL) {
		struct stat stat;
		fstat(fileDescriptor, &stat);
		for (int i = 0; i < cache->fileindex_size(); i++) {
			OsmAnd::OBF::FileIndex fi = cache->fileindex(i);
			if (hasEnding(inputName, fi.filename()) && fi.size() == stat.st_size) {
				fo = cache->mutable_fileindex(i);
				break;
			}
		}
	}
	if (fo != NULL) {
		mapFile->version = fo->version();
		mapFile->dateCreated = fo->datemodified();
		if (!routingOnly) {
			for (int i = 0; i < fo->mapindex_size(); i++) {
				MapIndex mi;
				OsmAnd::OBF::MapPart mp = fo->mapindex(i);
				mi.filePointer = mp.offset();
				mi.length = mp.size();
				mi.name = mp.name();
				for (int j = 0; j < mp.levels_size(); j++) {
					OsmAnd::OBF::MapLevel ml = mp.levels(j);
					MapRoot mr;
					mr.bottom = ml.bottom();
					mr.left = ml.left();
					mr.right = ml.right();
					mr.top = ml.top();
					mr.maxZoom = ml.maxzoom();
					mr.minZoom = ml.minzoom();
					mr.filePointer = ml.offset();
					mr.length = ml.size();
					mi.levels.push_back(mr);
				}
				mapFile->basemap = mapFile->basemap || mi.name.find("basemap") != string::npos;
				mapFile->mapIndexes.push_back(mi);
				mapFile->indexes.push_back(&mapFile->mapIndexes.back());
			}
		}

		for (int i = 0; i < fo->transportindex_size(); i++) {
			TransportIndex* ti = new TransportIndex();
			OsmAnd::OBF::TransportPart tp = fo->transportindex(i);
			ti->filePointer = tp.offset();
			ti->length = tp.size();
			ti->name = tp.name();
			ti->left = tp.left();
			ti->right = tp.right();
			ti->top = tp.top();
			ti->bottom = tp.bottom();
			IndexStringTable* st = new IndexStringTable();
			st->fileOffset = tp.stringtableoffset();
			st->length = tp.stringtablelength();
			ti->stringTable = st;
			ti->stopsFileOffset = tp.stopstableoffset();
			ti->stopsFileLength = tp.stopstablelength();
			ti->incompleteRoutesOffset = tp.incompleteroutesoffset();
			ti->incompleteRoutesLength = tp.incompleterouteslength();
			mapFile->transportIndexes.push_back(ti);
			mapFile->indexes.push_back(mapFile->transportIndexes.back());
		}

		for (int i = 0; i < fo->routingindex_size() && (!mapFile->liveMap || useLive); i++) {
			RoutingIndex* mi = new RoutingIndex();
			OsmAnd::OBF::RoutingPart mp = fo->routingindex(i);
			mi->filePointer = mp.offset();
			mi->length = mp.size();
			mi->name = mp.name();
			for (int j = 0; j < mp.subregions_size(); j++) {
				OsmAnd::OBF::RoutingSubregion ml = mp.subregions(j);
				RouteSubregion mr(mi);
				mr.bottom = ml.bottom();
				mr.left = ml.left();
				mr.right = ml.right();
				mr.top = ml.top();
				mr.mapDataBlock = ml.shiftodata();
				mr.filePointer = ml.offset();
				mr.length = ml.size();
				if (ml.basemap()) {
					mi->basesubregions.push_back(mr);
				} else {
					mi->subregions.push_back(mr);
				}
			}
			mapFile->routingIndexes.push_back(mi);
			mapFile->indexes.push_back(mapFile->routingIndexes.back());
		}
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Debug, "Native file initialized from cache %s", inputName.c_str());
	} else {
		FileInputStream input(fileDescriptor);
		input.SetCloseOnDelete(false);
		CodedInputStream cis(&input);
		cis.SetTotalBytesLimit(INT_MAXIMUM, INT_MAXIMUM >> 1);
		OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "File not initialized from cache : %s", inputName.c_str());
		if (!initMapStructure(&cis, mapFile, useLive, routingOnly)) {
			OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Error, "File not initialised : %s", inputName.c_str());
			delete mapFile;
			return NULL;
		}
	}

	openFiles.push_back(mapFile);
	transportIndexesList.insert(transportIndexesList.end(), mapFile->transportIndexes.begin(),
								mapFile->transportIndexes.end());
	return mapFile;
}

std::vector<BinaryMapFile*> getOpenMapFiles() {
	return openFiles;
}
