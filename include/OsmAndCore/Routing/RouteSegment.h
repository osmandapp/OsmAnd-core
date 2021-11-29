#ifndef _OSMAND_CORE_ROUTE_SEGMENT_H_
#define _OSMAND_CORE_ROUTE_SEGMENT_H_

#include <limits>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/Road.h>
#include <OsmAndCore/Routing/TurnInfo.h>

namespace OsmAnd {

    class RoutePlannerAnalyzer;
    class RoutePlanner;

    class OSMAND_CORE_API RouteSegment
    {
    private:
    protected:
        RouteSegment(const std::shared_ptr<const Road>& road, uint32_t startPointIndex, uint32_t endPointIndex);

        const std::shared_ptr<const Road> _road;
        uint32_t _startPointIndex;
        uint32_t _endPointIndex;
        QVector< QList< std::shared_ptr<RouteSegment> > > _attachedRoutes;

        float _distance;
        float _speed;
        float _time;
        QString _description;
        // this make not possible to make turns in between segment result for now
        TurnInfo _turnType;

        void dump(const QString& prefix = {}) const;
    public:
        virtual ~RouteSegment();

        const std::shared_ptr<const Road>& road;
        const uint32_t& startPointIndex;
        const uint32_t& endPointIndex;
        const QVector< QList< std::shared_ptr<RouteSegment> > >& attachedRoutes;

        const QString& description;
        const TurnInfo& turnInfo;

        const float& distance;
        const float& speed;
        const float& time;

        double getBearing(uint32_t pointIndex, bool isIncrement) const;
        double getBearingBegin() const;
        double getBearingEnd() const;

        friend class OsmAnd::RoutePlanner;
        friend class OsmAnd::RoutePlannerAnalyzer;
    };

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_ROUTE_PLANNER_H_)
