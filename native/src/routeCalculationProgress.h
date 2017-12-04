#ifndef _OSMAND_ROUTE_CALCULATION_PROGRESS_H
#define _OSMAND_ROUTE_CALCULATION_PROGRESS_H

class RouteCalculationProgress {
public:
	int segmentNotFound;
	float distanceFromBegin;
	int directSegmentQueueSize;
	float distanceFromEnd;
	int reverseSegmentQueueSize;
    float totalEstimatedDistance;
    
    float routingCalculatedTime;
    int visitedSegments;
    int loadedTiles;

	bool cancelled;
    
public:
	RouteCalculationProgress() : segmentNotFound(-1), distanceFromBegin(0),
		directSegmentQueueSize(0), distanceFromEnd(0),  reverseSegmentQueueSize(0), totalEstimatedDistance(0),
        routingCalculatedTime(0), visitedSegments(0), loadedTiles(0), cancelled(false){
	}

	virtual bool isCancelled(){
		return cancelled;
	}

	virtual void setSegmentNotFound(int s){
		segmentNotFound = s;
	}

	virtual void updateStatus(float distanceFromBegin,	int directSegmentQueueSize,	float distanceFromEnd,
			int reverseSegmentQueueSize) {
		this->distanceFromBegin = fmax(distanceFromBegin, this->distanceFromBegin );
		this->distanceFromEnd = fmax(distanceFromEnd,this->distanceFromEnd);
		this->directSegmentQueueSize = directSegmentQueueSize;
		this->reverseSegmentQueueSize = reverseSegmentQueueSize;
	}
};

#endif /*_OSMAND_ROUTE_CALCULATION_PROGRESS_H*/
