#ifndef _OSMAND_CORE_LOCATION_H_
#define _OSMAND_CORE_LOCATION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>

#include <QString>
#include <QStringBuilder>

namespace OsmAnd
{
    struct Location
    {
        double latitude, longitude, altitude;

        Location()
        {
            this->latitude = 0;
            this->longitude = 0;
            this->altitude = 0;
        }

        Location(const LatLon& that, const double altitude)
        {
            this->latitude = that.latitude;
            this->longitude = that.longitude;
            this->altitude = altitude;
        }

        Location(const double latitude, const double longitude, const double altitude)
        {
            this->latitude = latitude;
            this->longitude = longitude;
            this->altitude = altitude;
        }

        inline QString toQString() const
        {
            return QString::number(latitude) % ", " % QString::number(longitude)  % ", " % QString::number(altitude);
        }

#if !defined(SWIG)
        inline bool operator==(const Location& r) const
        {
            return qFuzzyCompare(latitude, r.latitude) && qFuzzyCompare(longitude, r.longitude) && qFuzzyCompare(altitude, r.altitude);
        }

        inline bool operator!=(const Location& r) const
        {
            return !qFuzzyCompare(latitude, r.latitude) || !qFuzzyCompare(longitude, r.longitude) || !qFuzzyCompare(altitude, r.altitude);
        }

        inline Location& operator=(const Location& that)
        {
            if (this != &that)
            {
                latitude = that.latitude;
                longitude = that.longitude;
                altitude = that.altitude;
            }
            return *this;
        }

        explicit inline operator PointD() const
        {
            return PointD(longitude, latitude);
        }
#endif // !defined(SWIG)
    };
}

#endif // !defined(_OSMAND_CORE_LOCATION_H_)
