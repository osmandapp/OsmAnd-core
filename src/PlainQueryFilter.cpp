#include "PlainQueryFilter.h"

OsmAnd::PlainQueryFilter::PlainQueryFilter(const ZoomLevel* zoom/* = nullptr*/, const AreaI* area/* = nullptr*/)
    : _isZoomFiltered(zoom != nullptr)
    , _zoom(_isZoomFiltered ? *zoom : ZoomLevel0)
    , _isAreaFiltered(area != nullptr)
    , _area(_isAreaFiltered ? *area : AreaI())
{
}

OsmAnd::PlainQueryFilter::~PlainQueryFilter()
{
}

bool OsmAnd::PlainQueryFilter::acceptsZoom(ZoomLevel zoom) const
{
    if(!_isZoomFiltered)
        return true;
    return _zoom == zoom;
}

bool OsmAnd::PlainQueryFilter::acceptsArea(const AreaI& area) const
{
    if(!_isAreaFiltered)
        return true;
    return
        _area.intersects(area) ||
        _area.contains(area) ||
        area.contains(_area);
}

bool OsmAnd::PlainQueryFilter::acceptsPoint(const PointI& point) const
{
    if(!_isAreaFiltered)
        return true;
    return _area.contains(point);
}
