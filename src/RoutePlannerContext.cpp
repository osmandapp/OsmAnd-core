#include "RoutePlanner.h"
#include "RoutePlannerContext.h"

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
    size_t memoryLimit  )
    : _useBasemap(useBasemap)
    , _memoryUsageLimit(memoryLimit)
    , _loadedTiles(0)
    , _initialHeading(initialHeading)
    , sources(sources)
    , configuration(routingConfig)
    , _routeStatistics(new RouteStatistics)
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
    ,_mixedLoadsCounter(0)
    , origin(origin)
{
}

OsmAnd::RoutePlannerContext::RoutingSubsectionContext::~RoutingSubsectionContext()
{
}



uint32_t OsmAnd::RoutePlannerContext::getCurrentlyLoadedTiles() {
    return _loadedTiles;
}

int intpow(int base, int pw) {
    int r = 1;
    for (int i = 0; i < pw; i++) {
        r *= base;
    }
    return r;
}


uint32_t OsmAnd::RoutePlannerContext::getCurrentEstimatedSize() {
    // TODO proper clculation
    return getCurrentlyLoadedTiles()*1000;
}

int compareSections(std::shared_ptr<OsmAnd::RoutePlannerContext::RoutingSubsectionContext> o1,
                    std::shared_ptr<OsmAnd::RoutePlannerContext::RoutingSubsectionContext> o2) {
    int v1 = (o1->getAccessCounter() + 1) * intpow(10, o1->getLoadsCounter() -1);
    int v2 = (o2->getAccessCounter() + 1) * intpow(10, o1->getLoadsCounter() -1);
    //return v1 < v2 ? -1 : (v1 == v2 ? 0 : 1);
    return v1 < v2;
}


void OsmAnd::RoutePlannerContext::unloadUnusedTiles(size_t memoryTarget) {
    float desirableSize = memoryTarget * 0.7f;
    QList< std::shared_ptr<RoutingSubsectionContext> > list;
    int loaded = 0;
    for(std::shared_ptr<RoutingSubsectionContext>  t : this->_subsectionsContexts) {
        if(t->isLoaded()) {
            list.append(t);
            loaded++;
        }
    }
    if(_routeStatistics) {
        _routeStatistics->maxLoadedTiles = qMax(_routeStatistics->maxLoadedTiles , getCurrentlyLoadedTiles());
    }
    qSort(list.begin(), list.end(), compareSections);

    int i = 0;
    while(getCurrentEstimatedSize() >= desirableSize && (list.size() - i) > loaded / 5 && i < list.size()) {
        std::shared_ptr<RoutingSubsectionContext>  unload = list[i];
        i++;
        unload->unload();
        if(_routeStatistics) {
            _routeStatistics->unloadedTiles ++;
        }

    }
    for(std::shared_ptr<OsmAnd::RoutePlannerContext::RoutingSubsectionContext> t : _subsectionsContexts) {
        t->_access /= 3;
    }
}

void OsmAnd::RoutePlannerContext::RoutingSubsectionContext::registerRoad( const std::shared_ptr<Model::Road>& road )
{
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
    return _mixedLoadsCounter  > 0;
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


            /*TODO: to Alexey what is this?
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
    const std::shared_ptr<RouteCalculationSegment>& original_)
{
    uint64_t id = (static_cast<uint64_t>(x31) << 31) | y31;
    auto itSegment = _roadSegments.find(id);
    if(itSegment == _roadSegments.end())
        return original_;
    this->_access++;

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
        LogPrintf(LogSeverityLevel::Debug, "%sroad(%llu), point(%d), w(%f), ds(%f), es(%f); parent = road(%llu), point(%d);",
            prefix.toStdString().c_str(),
            road->id, pointIndex, _distanceFromStart + _distanceToEnd, _distanceFromStart, _distanceToEnd,
            parent->road->id, parent->pointIndex);
    }
    else
    {
        LogPrintf(LogSeverityLevel::Debug, "%sroad(%llu), point(%d), w(%f), ds(%f), es(%f)",
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
