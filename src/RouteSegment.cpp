#include "RouteSegment.h"

#include <QtNumeric>
#include <QtCore>

#include "Utilities.h"
#include "Logging.h"

OsmAnd::RouteSegment::RouteSegment(const std::shared_ptr<Model::Road>& road_, uint32_t startPointIndex_, uint32_t endPointIndex_)
    : _road(road_)
    , _startPointIndex(startPointIndex_)
    , _endPointIndex(endPointIndex_)
    , road(_road)
    , startPointIndex(_startPointIndex)
    , endPointIndex(_endPointIndex)
    , attachedRoutes(_attachedRoutes)
    , distance(_distance)
    , speed(_speed)
    , time(_time)
{
    if(startPointIndex_ >= road_->points.size() || endPointIndex_ >= road_->points.size())
    {
        int i = 5;
    }
    _attachedRoutes.resize(qAbs(static_cast<int64_t>(_endPointIndex) - static_cast<int64_t>(_startPointIndex)) + 1);
}

OsmAnd::RouteSegment::~RouteSegment()
{
}

double OsmAnd::RouteSegment::getBearing( uint32_t pointIndex, bool isIncrement ) const
{
    return road->getDirectionDelta(pointIndex, isIncrement) / M_PI * 180.0;
}

double OsmAnd::RouteSegment::getBearingBegin() const
{
    return road->getDirectionDelta(_startPointIndex, _startPointIndex < _endPointIndex) / M_PI * 180.0;
}

double OsmAnd::RouteSegment::getBearingEnd() const
{
    return Utilities::normalizedAngleRadians(road->getDirectionDelta(_endPointIndex, _startPointIndex > _endPointIndex) - M_PI) / M_PI * 180.0;
}

void OsmAnd::RouteSegment::dump( const QString& prefix /*= QString()*/ ) const
{
    LogPrintf(LogSeverityLevel::Debug, "%sroad(%llu), [%u:%u]", prefix.toStdString().c_str(), road->id, _startPointIndex, _endPointIndex);
}

