#include "RoutePlannerContext.h"

#include "RoutePlanner.h"
#include "Logging.h"

#include "Utilities.h"
#include "ObfReader.h"

OsmAnd::RoutePlannerContext::RoutePlannerContext(
    const QList< std::shared_ptr<ObfReader> >& sources,
    const std::shared_ptr<RoutingConfiguration>& routingConfig,
    const QString& vehicle,
    bool useBasemap,
    float initialHeading /*= std::numeric_limits<float>::quiet_NaN()*/,
    QHash<QString, QString>* options /*=nullptr*/,
    size_t memoryLimit /*= std::numeric_limits<size_t>::max()*/ )
    : _useBasemap(useBasemap)
    , _memoryUsageLimit(memoryLimit)
    , _initialHeading(initialHeading)
    , sources(sources)
    , configuration(routingConfig)
    , profileContext(new RoutingProfileContext(configuration->routingProfiles[vehicle], options))
{
    _partialRecalculationDistanceLimit = Utilities::parseArbitraryFloat(configuration->resolveAttribute(vehicle, "recalculateDistanceHelp"), 10000.0f);
    _heuristicCoefficient = Utilities::parseArbitraryFloat(configuration->resolveAttribute(vehicle, "heuristicCoefficient"), 1.0f);
    _planRoadDirection = Utilities::parseArbitraryInt(configuration->resolveAttribute(vehicle, "planRoadDirection"), 0);
    _roadTilesLoadingZoomLevel = Utilities::parseArbitraryUInt(configuration->resolveAttribute(vehicle, "zoomToLoadTiles"), DefaultRoadTilesLoadingZoomLevel);

    for(auto itSource = sources.begin(); itSource != sources.end(); ++itSource)
    {
        auto source = *itSource;

        for(auto itRoutingSection = source->routingSections.begin(); itRoutingSection != source->routingSections.end(); ++itRoutingSection)
            _sourcesLUT.insert((*itRoutingSection).get(), source);
    }
}

OsmAnd::RoutePlannerContext::~RoutePlannerContext()
{
}

OsmAnd::RoutePlannerContext::RoutingSubsectionContext::RoutingSubsectionContext( RoutePlannerContext* owner, const std::shared_ptr<ObfReader>& origin, const std::shared_ptr<ObfRoutingSection::Subsection>& subsection )
    : subsection(subsection)
    , owner(owner)
    , origin(origin)
{
}

OsmAnd::RoutePlannerContext::RoutingSubsectionContext::~RoutingSubsectionContext()
{
}

void OsmAnd::RoutePlannerContext::RoutingSubsectionContext::registerRoad( const std::shared_ptr<Model::Road>& road )
{
    //TODO:tileStatistics.addObject(ro);

    uint32_t idx = 0;
    for(auto itPoint = road->points.begin(); itPoint != road->points.end(); ++itPoint, idx++)
    {
        const auto& x31 = itPoint->x;
        const auto& y31 = itPoint->y;
        uint64_t id = (static_cast<uint64_t>(x31) << 31) | y31;
        
        std::shared_ptr<RouteCalculationSegment> routeSegment(new RouteCalculationSegment(road, idx));
        auto itRouteSegment = _roadSegments.find(id);
        if(itRouteSegment == _roadSegments.end())
            _roadSegments.insert(id, routeSegment);
        else
        {
            auto originalRouteSegment = *itRouteSegment;
            while(originalRouteSegment->_next)
                originalRouteSegment = originalRouteSegment->_next;
            originalRouteSegment->_next = routeSegment;
        }
    }
}

bool OsmAnd::RoutePlannerContext::RoutingSubsectionContext::isLoaded() const
{
    return (_mixedLoadsCounter & 0x80000000) == 0;
}

uint32_t OsmAnd::RoutePlannerContext::RoutingSubsectionContext::getLoadsCounter() const
{
    return _mixedLoadsCounter & 0x7fffffff;
}

void OsmAnd::RoutePlannerContext::RoutingSubsectionContext::collectRoads( QList< std::shared_ptr<Model::Road> >& output, QMap<uint64_t, std::shared_ptr<Model::Road> >* duplicatesRegistry /*= nullptr*/ )
{
    for(auto itRouteSegment = _roadSegments.begin(); itRouteSegment != _roadSegments.end(); ++itRouteSegment)
    {
        auto routeSegment = itRouteSegment.value();
        while(routeSegment)
        {
            auto road = routeSegment->road;

            const auto isDuplicate = duplicatesRegistry && duplicatesRegistry->contains(road->id);
            if(!isDuplicate)
            {
                if(duplicatesRegistry)
                    duplicatesRegistry->insert(road->id, road);
                output.push_back(road);
            }

            routeSegment = routeSegment->_next;
        }
    }


            /*TODO:
        if(routes != null) {
        } else if(searchResult != null) {
            RouteDataObject[] objects = searchResult.objects;
            if(objects != null) {
                for(RouteDataObject ro : objects) {
                    if (ro != null && !excludeDuplications.contains(ro.id)) {
                        excludeDuplications.put(ro.id, ro);
                        toFillIn.add(ro);
                    }
                }
            }
        }
        */
}

void OsmAnd::RoutePlannerContext::RoutingSubsectionContext::markLoaded()
{
    _mixedLoadsCounter = (_mixedLoadsCounter & 0x7fffffff) + 1;
}

void OsmAnd::RoutePlannerContext::RoutingSubsectionContext::unload()
{
    _mixedLoadsCounter |= 0x80000000;
}

std::shared_ptr<OsmAnd::RoutePlannerContext::RouteCalculationSegment> OsmAnd::RoutePlannerContext::RoutingSubsectionContext::loadRouteCalculationSegment(
    uint32_t x31, uint32_t y31,
    QMap<uint64_t, std::shared_ptr<Model::Road> >& processed,
    const std::shared_ptr<RouteCalculationSegment>& original_) const
{
    uint64_t id = (static_cast<uint64_t>(x31) << 31) | y31;
    auto itSegment = _roadSegments.find(id);
    if(itSegment == _roadSegments.end())
        return original_;

    auto original = original_;
    auto segment = *itSegment;
    while(segment)
    {
        auto road = segment->road;
        auto roadPointId = RoutePlanner::encodeRoutePointId(road, segment->pointIndex);
        auto itOtherRoad = processed.find(roadPointId);
        if(itOtherRoad == processed.end() || (*itOtherRoad)->points.size() < road->points.size())
        {
            processed.insert(roadPointId, road);

            std::shared_ptr<RouteCalculationSegment> newSegment(new RouteCalculationSegment(road, segment->pointIndex));
            newSegment->_next = original;
            original = newSegment;
        }

        segment = segment->next;
    }
    
    return original;
}

OsmAnd::RoutePlannerContext::CalculationContext::CalculationContext( RoutePlannerContext* owner )
    : owner(owner)
{
}

OsmAnd::RoutePlannerContext::CalculationContext::~CalculationContext()
{
}

OsmAnd::RoutePlannerContext::RouteCalculationSegment::RouteCalculationSegment( const std::shared_ptr<Model::Road>& road, uint32_t pointIndex )
    : _distanceFromStart(0)
    , _distanceToEnd(0)
    , next(_next)
    , parent(_parent)
    , parentEndPointIndex(_parentEndPointIndex)
    , road(road)
    , pointIndex(pointIndex)
    , _allowedDirection(0)
    , _assignedDirection(0)
{
}

OsmAnd::RoutePlannerContext::RouteCalculationSegment::~RouteCalculationSegment()
{
}

void OsmAnd::RoutePlannerContext::RouteCalculationSegment::dump(const QString& prefix /*= QString()*/) const
{
    if(parent)
    {
        LogPrintf(LogSeverityLevel::Debug, "%sroad(%llu), point(%d), w(%f), ds(%f), es(%f); parent = road(%llu), point(%d);\n",
            prefix.toStdString().c_str(),
            road->id, pointIndex, _distanceFromStart + _distanceToEnd, _distanceFromStart, _distanceToEnd,
            parent->road->id, parent->pointIndex);
    }
    else
    {
        LogPrintf(LogSeverityLevel::Debug, "%sroad(%llu), point(%d), w(%f), ds(%f), es(%f)\n",
            prefix.toStdString().c_str(),
            road->id, pointIndex, _distanceFromStart + _distanceToEnd, _distanceFromStart, _distanceToEnd);
    }
}

OsmAnd::RoutePlannerContext::RouteCalculationFinalSegment::RouteCalculationFinalSegment( const std::shared_ptr<Model::Road>& road, uint32_t pointIndex )
    : RouteCalculationSegment(road, pointIndex)
    , reverseWaySearch(_reverseWaySearch)
    , opposite(_opposite)
{
}

OsmAnd::RoutePlannerContext::RouteCalculationFinalSegment::~RouteCalculationFinalSegment()
{

}
