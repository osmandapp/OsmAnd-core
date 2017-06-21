#ifndef _OSMAND_PRECALCULATED_ROUTE_DIRECTION_H
#define _OSMAND_PRECALCULATED_ROUTE_DIRECTION_H
#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct RoutingContext;
struct RouteSegmentResult;

struct PrecalculatedRouteDirection {
	vector<uint32_t> pointsX;
	vector<uint32_t> pointsY;
	vector<float> times;
	float minSpeed;
	float maxSpeed;
	float startFinishTime;
	float endFinishTime;
	bool followNext;
	static int SHIFT;
	static int SHIFTS[];
	bool empty;

	uint64_t startPoint;
	uint64_t endPoint;
	quad_tree<int> quadTree;

 	inline uint64_t calc(int x31, int y31) {
		return (((uint64_t) x31) << 32l) + ((uint64_t)y31);
	}

public:
    PrecalculatedRouteDirection() {
    }

private:
    PrecalculatedRouteDirection(vector<RouteSegmentResult>& ls, float maxSpeed);

    PrecalculatedRouteDirection(vector<int>& x31, vector<int>& y31, float maxSpeed);
    
    PrecalculatedRouteDirection(PrecalculatedRouteDirection& parent, int s1, int s2);

private:
    void init(vector<int>& x31, vector<int>& y31);
    void init(vector<int>& x31, vector<int>& y31, vector<float>& speedSegments);
    void init(vector<RouteSegmentResult>& ls);

public:
    static SHARED_PTR<PrecalculatedRouteDirection> build(vector<RouteSegmentResult>& ls, float cutoffDistance, float maxSpeed);
    static SHARED_PTR<PrecalculatedRouteDirection> build(vector<int>& x31, vector<int>& y31, float maxSpeed);

    SHARED_PTR<PrecalculatedRouteDirection> adopt(RoutingContext* ctx);
    
	float getDeviationDistance(int x31, int y31, int ind);
	float getDeviationDistance(int x31, int y31);
	int getIndex(int x31, int y31);
	float timeEstimate(int begX, int begY, int endX, int endY);

};

#endif /*_OSMAND_PRECALCULATED_ROUTE_DIRECTION_H*/
