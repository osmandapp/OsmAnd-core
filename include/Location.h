/*
 * Location.h
 *
 *  Created on: Mar 15, 2013
 *      Author: victor
 */

#ifndef LOCATION_H_
#define LOCATION_H_

#include <qstring.h>
#include <math.h>
#include <OsmAndCore.h>

namespace OsmAnd {

void computeDistanceAndBearing(double lat1, double lon1, double lat2, double lon2,
		double* distance, double* bearing = nullptr, double* finalBearing = nullptr);

class OSMAND_CORE_API Location {
public:
	QString mProvider;
    long long mTime;
    double mLatitude ;
    double mLongitude;
    bool mHasAltitude;
    double mAltitude ;
    bool mHasSpeed;
    float mSpeed ;
    bool mHasBearing ;
    float mBearing ;
    bool mHasAccuracy ;
    float mAccuracy ;
public:
	Location(QString& provider) :
			mProvider(provider), mHasAccuracy(false), mHasAltitude(false),
			mHasBearing(false), mHasSpeed(false), mTime(0) {
		mAccuracy = mBearing = mSpeed = mAltitude = 0;
		mLatitude = mLongitude = 0;

	}
	double bearingTo(Location& loc) {
		double d;
		double b;
		computeDistanceAndBearing(mLatitude, mLongitude, loc.mLatitude, loc.mLongitude, &d, &b, nullptr);
		return b;
	}

	double distanceTo(Location& loc) {
		double d;
		computeDistanceAndBearing(mLatitude, mLongitude, loc.mLatitude, loc.mLongitude, &d, nullptr, nullptr);
		return d;
	}
	virtual ~Location() {};
};


} /* namespace OsmAnd */
#endif /* LOCATION_H_ */
