#ifndef _OSMAND_TRANSPORT_ROUTING_OBJECTS_H
#define _OSMAND_TRANSPORT_ROUTING_OBJECTS_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "boost/functional/hash.hpp"


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

	bool compareNode(Node& thatObj);

	bool operator==(const Node &n) const
	{
		return id == n.id && lat == n.lat && lon == n.lon;
	}
};

struct Way
{
	int64_t id;
	vector<Node> nodes;
	vector<int64_t> nodeIds;

	Way();

	Way(int64_t id_);

	Way(Way &w_);

	Way(const Way &w_);

	void addNode(Node& n);
	Node getFirstNode();
	int64_t getFirstNodeId();
	Node getLastNode();
	int64_t getLastNodeId();
	void reverseNodes();
	bool compareWay(Way& thatObj);
	
	
	bool operator==(const Way &w) const
	{
		return id == w.id && nodes == w.nodes && nodeIds == w.nodeIds;
	}

};

namespace std {

template <>
struct hash<Way>
{
    std::size_t operator()(const Way& w) const
    {
        std::size_t result = 0;
        boost::hash_combine(result, w.id);
        return result;
        
    }
};

template <>
struct hash<Node>
{
    std::size_t operator()(const Node& n) const
    {
        std::size_t result = 0;
        boost::hash_combine(result, n.id);
        boost::hash_combine(result, n.lat);
        boost::hash_combine(result, n.lon);
        
        return result;
    }
};

}

struct TransportRoute : public MapObject
{
	vector<SHARED_PTR<TransportStop>> forwardStops;
	string ref;
	string routeOperator;
	string type;
	uint32_t dist;
	string color;
	vector<Way> forwardWays; 
	SHARED_PTR<TransportSchedule> schedule;

	TransportRoute();

	SHARED_PTR<TransportSchedule> getOrCreateSchedule();
	void mergeForwardWays();
	void addWay(Way& w);
	int32_t getAvgBothDistance();
	int32_t getDist();
	string getAdjustedRouteRef(bool small);
	bool compareRoute(SHARED_PTR<TransportRoute>& thatObj);

	
};

#endif //_OSMAND_TRANSPORT_ROUTING_OBJECTS_H
