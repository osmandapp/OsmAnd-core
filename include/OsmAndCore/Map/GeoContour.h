#ifndef _OSMAND_CORE_GEO_CONTOUR_H_
#define _OSMAND_CORE_GEO_CONTOUR_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QVector>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API GeoContour
    {
        Q_DISABLE_COPY_AND_MOVE(GeoContour);

    private:
    protected:
    public:
        GeoContour(const double value, const QVector<PointI>& points);
        virtual ~GeoContour();
        
        const double value;
        const QVector<PointI> points;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_CONTOUR_H_)
