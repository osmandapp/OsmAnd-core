#include "GeoContour.h"

OsmAnd::GeoContour::GeoContour(
    const double value_,
    const QVector<PointI>& points_)
    : value(value_)
    , points(points_)
{
}

OsmAnd::GeoContour::~GeoContour()
{
}
