#ifndef _OSMAND_TRANSPORT_ROUTING_OBJECTS_H
#define _OSMAND_TRANSPORT_ROUTING_OBJECTS_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

#define SAME_STOP 40

const static int TRANSPORT_STOP_ZOOM = 24;

struct TransportRoute;

struct MapObject
{
	int64_t id;
	double lat;
	double lon;
	string name;
	string enName;
	UNORDERED(map)<string, string> names;
	int32_t fileOffset;

	MapObject();

	UNORDERED(map)<string, string> getNamesMap(bool includeEn);
	string getName(string lang);
};

struct TransportStopExit
{
	int x31;
	int y31;
	string ref;

	TransportStopExit();

	void setLocation(int zoom, int32_t dx, int32_t dy);
	bool compareExit(SHARED_PTR<TransportStopExit> &thatObj);
};

struct TransportStop : public MapObject
{
	const static int32_t DELETED_STOP = -1;

	vector<int32_t> referencesToRoutes;
	vector<int64_t> deletedRoutesIds;
	vector<int64_t> routesIds;
	int32_t distance;
	int32_t x31;
	int32_t y31;
	vector<SHARED_PTR<TransportStopExit>> exits;
	vector<SHARED_PTR<TransportRoute>> routes;
	UNORDERED(map)<string, vector<int32_t>> referencesToRoutesMap; //add linked realizations?

	TransportStop();

	bool hasRoute(int64_t routeId);
	bool isDeleted();
	bool isRouteDeleted(int64_t routeId);
	bool hasReferencesToRoutes();
	void putReferenceToRoutes(string &repositoryFileName, vector<int32_t> &referencesToRoutes);
	void addRouteId(int64_t routeId);
	void addRoute(SHARED_PTR<TransportRoute> rt);
	bool compareStop(SHARED_PTR<TransportStop> &thatObj);
	void setLocation(int zoom, int32_t dx, int32_t dy);
};

struct TransportSchedule
{
	vector<int32_t> tripIntervals;
	vector<int32_t> avgStopIntervals;
	vector<int32_t> avgWaitIntervals;
	
	TransportSchedule();

	bool compareSchedule(const SHARED_PTR<TransportSchedule> &thatObj);
};

struct Node
{
	int64_t id;
	double lat;
	double lon;

	Node(double lat_, double lon_, int64_t id_);

	bool compareNode(SHARED_PTR<Node> thatObj);
};

struct Way
{
	int64_t id;
	vector<SHARED_PTR<Node>> nodes;
	vector<int64_t> nodeIds;

	Way(int64_t id_);

	Way(Way *w_);

	void addNode(SHARED_PTR<Node> n);
	SHARED_PTR<Node> getFirstNode();
	int64_t getFirstNodeId();
	SHARED_PTR<Node> getLastNode();
	int64_t getLastNodeId();
	void reverseNodes();
	bool compareWay(SHARED_PTR<Way> thatObj);
};

struct TransportRoute : public MapObject
{
	vector<SHARED_PTR<TransportStop>> forwardStops;
	string ref;
	string routeOperator;
	string type;
	uint32_t dist;
	string color;
	vector<SHARED_PTR<Way>> forwardWays; //todo is there a Way or analogue?
	SHARED_PTR<TransportSchedule> schedule;

	TransportRoute();

	SHARED_PTR<TransportSchedule> getOrCreateSchedule();
	void mergeForwardWays();
	void addWay(SHARED_PTR<Way> w);
	int32_t getAvgBothDistance();
	int32_t getDist();
	string getAdjustedRouteRef(bool small);
	bool compareRoute(SHARED_PTR<TransportRoute> thatObj);

	
};

#endif //_OSMAND_TRANSPORT_ROUTING_OBJECTS_H
