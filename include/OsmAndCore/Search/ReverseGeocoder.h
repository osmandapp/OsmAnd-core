#ifndef REVERSEGEOCODER_H
#define REVERSEGEOCODER_H

#include "QtExtensions.h"
#include "LatLon.h"
#include "GeocodingResult.h"
#include "RoadLocator.h"

#include <QVector>

namespace OsmAnd
{
    class OSMAND_CORE_API ReverseGeocoder
    {
        Q_DISABLE_COPY_AND_MOVE(ReverseGeocoder)

    public:
        QVector<GeocodingResult> reverseGeocodingSearch(const RoadLocator &roadLocator, LatLon latLon);
    };
}

#endif // REVERSEGEOCODER_H
