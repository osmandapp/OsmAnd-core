#ifndef _OSMAND_ROUTE_CALCULATION_PROGRESS_H
#define _OSMAND_ROUTE_CALCULATION_PROGRESS_H

#include <algorithm>

class RouteCalculationProgress {
public:
	OsmAnd::ElapsedTimer timeToLoad;
	OsmAnd::ElapsedTimer timeToCalculate;
	OsmAnd::ElapsedTimer timeExtra;

	int segmentNotFound;
	float distanceFromBegin;
	int directSegmentQueueSize;
	float distanceFromEnd;
	int reverseSegmentQueueSize;
	float totalEstimatedDistance;
	
	float routingCalculatedTime;
	int visitedSegments;
	int loadedTiles;

	int visitedDirectSegments;
	int visitedOppositeSegments;
	int directQueueSize;
	int oppositeQueueSize;

	bool cancelled;
	
public:
	RouteCalculationProgress() : segmentNotFound(-1), distanceFromBegin(0),
		directSegmentQueueSize(0), distanceFromEnd(0),  reverseSegmentQueueSize(0), totalEstimatedDistance(0),
		routingCalculatedTime(0), visitedSegments(0), loadedTiles(0), 
		visitedDirectSegments(0), visitedOppositeSegments(0), directQueueSize(0),  oppositeQueueSize(0), cancelled(false) {
	}

	virtual bool isCancelled(){
		return cancelled;
	}

	virtual void setSegmentNotFound(int s){
		segmentNotFound = s;
	}

	virtual void updateStatus(float distanceFromBegin,	int directSegmentQueueSize,	float distanceFromEnd,
			int reverseSegmentQueueSize) {
		this->distanceFromBegin = std::max(distanceFromBegin, this->distanceFromBegin);
		this->distanceFromEnd = std::max(distanceFromEnd,this->distanceFromEnd);
		this->directSegmentQueueSize = directSegmentQueueSize;
		this->reverseSegmentQueueSize = reverseSegmentQueueSize;
	}
};

#endif /*_OSMAND_ROUTE_CALCULATION_PROGRESS_H*/
