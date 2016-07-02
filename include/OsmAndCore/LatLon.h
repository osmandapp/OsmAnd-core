#ifndef _OSMAND_CORE_LATLON_H_
#define _OSMAND_CORE_LATLON_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>

#include <QString>
#include <QStringBuilder>

 
namespace OsmAnd
{
    struct LatLon
    {
        double latitude, longitude;

        inline LatLon()
        {
            this->latitude = 0;
            this->longitude = 0;
        }

        inline LatLon(const LatLon& that)
        {
            this->latitude = that.latitude;
            this->longitude = that.longitude;
        }

        inline LatLon(const double latitude, const double longitude)
        {
            this->latitude = latitude;
            this->longitude = longitude;
        }

		inline QString toQString() const
		{
			return QString::number(latitude) % ", " % QString::number(longitude);
		}

#if !defined(SWIG)
        inline bool operator==(const LatLon& r) const
        {
            return qFuzzyCompare(latitude, r.latitude) && qFuzzyCompare(longitude, r.longitude);
        }

        inline bool operator!=(const LatLon& r) const
        {
            return !qFuzzyCompare(latitude, r.latitude) || !qFuzzyCompare(longitude, r.longitude);
        }

        inline LatLon& operator=(const LatLon& that)
        {
            if (this != &that)
            {
                latitude = that.latitude;
                longitude = that.longitude;
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

#endif // !defined(_OSMAND_CORE_LATLON_H_)
