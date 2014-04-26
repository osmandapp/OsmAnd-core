#include "FunctorQueryFilter.h"

OsmAnd::FunctorQueryFilter::FunctorQueryFilter(
    const AcceptsZoomCallback acceptsZoomCallback_,
    const AcceptsAreaCallback acceptsAreaCallback_,
    const AcceptsPointCallback acceptsPointCallback_)
    : _acceptsZoomCallback(acceptsZoomCallback_)
    , _acceptsAreaCallback(acceptsAreaCallback_)
    , _acceptsPointCallback(acceptsPointCallback_)
{
}

OsmAnd::FunctorQueryFilter::~FunctorQueryFilter()
{
}

bool OsmAnd::FunctorQueryFilter::acceptsZoom(ZoomLevel zoom) const
{
    if(!_acceptsZoomCallback)
        return true;
    return _acceptsZoomCallback(zoom);
}

bool OsmAnd::FunctorQueryFilter::acceptsArea(const AreaI& area) const
{
    if(!_acceptsAreaCallback)
        return true;
    return _acceptsAreaCallback(area);
}

bool OsmAnd::FunctorQueryFilter::acceptsPoint(const PointI& point) const
{
    if(!_acceptsPointCallback)
        return true;
    return _acceptsPointCallback(point);
}
