#include "LambdaQueryFilter.h"

OsmAnd::LambdaQueryFilter::LambdaQueryFilter( ZoomFunctionSignature zoomFunction, AreaFunctionSignature areaFunction, PointFunctionSignature pointFunction )
    : _zoomFunction(zoomFunction)
    , _areaFunction(areaFunction)
    , _pointFunction(pointFunction)
{
}

OsmAnd::LambdaQueryFilter::~LambdaQueryFilter()
{
}

bool OsmAnd::LambdaQueryFilter::acceptsZoom(ZoomLevel zoom)
{
    if(!_zoomFunction)
        return true;
    return _zoomFunction(zoom);
}

bool OsmAnd::LambdaQueryFilter::acceptsArea( const AreaI& area )
{
    if(!_areaFunction)
        return true;
    return _areaFunction(area);
}

bool OsmAnd::LambdaQueryFilter::acceptsPoint( const PointI& point )
{
    if(!_pointFunction)
        return true;
    return _pointFunction(point);
}
