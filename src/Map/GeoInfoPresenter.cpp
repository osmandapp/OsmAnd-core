#include "GeoInfoPresenter.h"
#include "GeoInfoPresenter_P.h"

#include "Utilities.h"

OsmAnd::GeoInfoPresenter::GeoInfoPresenter(
    const QList< std::shared_ptr<const GeoInfoDocument> >& documents_)
    : _p(new GeoInfoPresenter_P(this))
    , documents(documents_)
{
}

OsmAnd::GeoInfoPresenter::~GeoInfoPresenter()
{
}

std::shared_ptr<OsmAnd::IMapObjectsProvider> OsmAnd::GeoInfoPresenter::createMapObjectsProvider() const
{
    return _p->createMapObjectsProvider();
}

std::shared_ptr<const OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules> OsmAnd::GeoInfoPresenter::MapObject::defaultEncodingDecodingRules(OsmAnd::modifyAndReturn(
    std::shared_ptr<OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules>(new OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules()),
    (std::function<void(std::shared_ptr<OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules>& instance)>)[]
    (std::shared_ptr<OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules>& rules) -> void
    {
        rules->verifyRequiredRulesExist();
    }));

OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules::EncodingDecodingRules()
    : waypoint_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , trackpoint_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , routepoint_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , trackline_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , routeline_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , waypointsPresent_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , waypointsNotPresent_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , trackpointsPresent_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , trackpointsNotPresent_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , routepointsPresent_encodingRuleId(std::numeric_limits<uint32_t>::max())
    , routepointsNotPresent_encodingRuleId(std::numeric_limits<uint32_t>::max())
{
}

OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules::~EncodingDecodingRules()
{
}

void OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules::createRequiredRules(uint32_t& lastUsedRuleId)
{
    if (waypoint_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("osmand"), QLatin1String("file_waypoint"));
    }

    if (trackpoint_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("osmand"), QLatin1String("file_trackpoint"));
    }

    if (routepoint_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("osmand"), QLatin1String("file_routepoint"));
    }

    if (trackline_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("osmand"), QLatin1String("file_trackline"));
    }

    if (routeline_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("osmand"), QLatin1String("file_routeline"));
    }

    if (routeline_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("osmand"), QLatin1String("file_routeline"));
    }

    if (waypointsPresent_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("file_has_waypoints"), QLatin1String("true"));
    }

    if (waypointsNotPresent_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("file_has_waypoints"), QLatin1String("false"));
    }

    if (trackpointsPresent_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("file_has_trackpoints"), QLatin1String("true"));
    }

    if (trackpointsNotPresent_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("file_has_trackpoints"), QLatin1String("false"));
    }

    if (routepointsPresent_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("file_has_routepoints"), QLatin1String("true"));
    }

    if (routepointsNotPresent_encodingRuleId == std::numeric_limits<uint32_t>::max())
    {
        addRule(lastUsedRuleId++,
            QLatin1String("file_has_routepoints"), QLatin1String("false"));
    }
}

uint32_t OsmAnd::GeoInfoPresenter::MapObject::EncodingDecodingRules::addRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue)
{
    OsmAnd::MapObject::EncodingDecodingRules::addRule(ruleId, ruleTag, ruleValue);

    if (QLatin1String("osmand") == ruleTag && QLatin1String("file_waypoint") == ruleValue)
        waypoint_encodingRuleId = ruleId;
    else if (QLatin1String("osmand") == ruleTag && QLatin1String("file_trackpoint") == ruleValue)
        trackpoint_encodingRuleId = ruleId;
    else if (QLatin1String("osmand") == ruleTag && QLatin1String("file_routepoint") == ruleValue)
        routepoint_encodingRuleId = ruleId;
    else if (QLatin1String("osmand") == ruleTag && QLatin1String("file_trackline") == ruleValue)
        trackline_encodingRuleId = ruleId;
    else if (QLatin1String("osmand") == ruleTag && QLatin1String("file_routeline") == ruleValue)
        routeline_encodingRuleId = ruleId;
    else if (QLatin1String("file_has_waypoints") == ruleTag && QLatin1String("true") == ruleValue)
        waypointsPresent_encodingRuleId = ruleId;
    else if (QLatin1String("file_has_waypoints") == ruleTag && QLatin1String("false") == ruleValue)
        waypointsNotPresent_encodingRuleId = ruleId;
    else if (QLatin1String("file_has_trackpoints") == ruleTag && QLatin1String("true") == ruleValue)
        trackpointsPresent_encodingRuleId = ruleId;
    else if (QLatin1String("file_has_trackpoints") == ruleTag && QLatin1String("false") == ruleValue)
        trackpointsNotPresent_encodingRuleId = ruleId;
    else if (QLatin1String("file_has_routepoints") == ruleTag && QLatin1String("true") == ruleValue)
        routepointsPresent_encodingRuleId = ruleId;
    else if (QLatin1String("file_has_routepoints") == ruleTag && QLatin1String("false") == ruleValue)
        routepointsNotPresent_encodingRuleId = ruleId;

    return ruleId;
}

OsmAnd::GeoInfoPresenter::MapObject::MapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_)
    : geoInfoDocument(geoInfoDocument_)
{
    encodingDecodingRules = defaultEncodingDecodingRules;

    additionalTypesRuleIds.append(geoInfoDocument->locationMarks.isEmpty()
        ? defaultEncodingDecodingRules->waypointsNotPresent_encodingRuleId
        : defaultEncodingDecodingRules->waypointsPresent_encodingRuleId);

    additionalTypesRuleIds.append(geoInfoDocument->tracks.isEmpty()
        ? defaultEncodingDecodingRules->trackpointsNotPresent_encodingRuleId
        : defaultEncodingDecodingRules->trackpointsPresent_encodingRuleId);

    additionalTypesRuleIds.append(geoInfoDocument->routes.isEmpty()
        ? defaultEncodingDecodingRules->routepointsNotPresent_encodingRuleId
        : defaultEncodingDecodingRules->routepointsPresent_encodingRuleId);
}

OsmAnd::GeoInfoPresenter::MapObject::~MapObject()
{
}

OsmAnd::GeoInfoPresenter::WaypointMapObject::WaypointMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::LocationMark>& waypoint_)
    : MapObject(geoInfoDocument_)
    , waypoint(waypoint_)
{
    const auto position31 = Utilities::convertLatLonTo31(waypoint->position);
    points31.push_back(position31);
    bbox31.topLeft = bbox31.bottomRight = position31;

    if (!waypoint->name.isEmpty())
    {
        captionsOrder.push_back(defaultEncodingDecodingRules->name_encodingRuleId);
        captions[defaultEncodingDecodingRules->name_encodingRuleId] = waypoint->name;
    }

    typesRuleIds.append(defaultEncodingDecodingRules->waypoint_encodingRuleId);
}

OsmAnd::GeoInfoPresenter::WaypointMapObject::~WaypointMapObject()
{
}

OsmAnd::GeoInfoPresenter::TrackpointMapObject::TrackpointMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::Track>& track_,
    const std::shared_ptr<const GeoInfoDocument::TrackSegment>& trackSegment_,
    const std::shared_ptr<const GeoInfoDocument::TrackPoint>& trackpoint_)
    : MapObject(geoInfoDocument_)
    , track(track_)
    , trackSegment(trackSegment_)
    , trackpoint(trackpoint_)
{
    const auto position31 = Utilities::convertLatLonTo31(trackpoint->position);
    points31.push_back(position31);
    bbox31.topLeft = bbox31.bottomRight = position31;

    if (!trackpoint->name.isEmpty())
    {
        captionsOrder.push_back(defaultEncodingDecodingRules->name_encodingRuleId);
        captions[defaultEncodingDecodingRules->name_encodingRuleId] = trackpoint->name;
    }

    typesRuleIds.append(defaultEncodingDecodingRules->trackpoint_encodingRuleId);
}

OsmAnd::GeoInfoPresenter::TrackpointMapObject::~TrackpointMapObject()
{
    typesRuleIds.append(defaultEncodingDecodingRules->trackpoint_encodingRuleId);
}

OsmAnd::GeoInfoPresenter::TracklineMapObject::TracklineMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::Track>& track_,
    const std::shared_ptr<const GeoInfoDocument::TrackSegment>& trackSegment_)
    : MapObject(geoInfoDocument_)
    , track(track_)
    , trackSegment(trackSegment_)
{
    if (!trackSegment->points.isEmpty())
    {
        points31.resize(trackSegment->points.size());
        auto pPosition31 = points31.data();
        for (const auto& trackpoint : constOf(trackSegment->points))
            *(pPosition31++) = Utilities::convertLatLonTo31(trackpoint->position);
        computeBBox31();
    }
    
    if (!track->name.isEmpty())
    {
        captionsOrder.push_back(defaultEncodingDecodingRules->name_encodingRuleId);
        captions[defaultEncodingDecodingRules->name_encodingRuleId] = track->name;
    }

    typesRuleIds.append(defaultEncodingDecodingRules->trackline_encodingRuleId);
}

OsmAnd::GeoInfoPresenter::TracklineMapObject::~TracklineMapObject()
{
}

OsmAnd::GeoInfoPresenter::RoutepointMapObject::RoutepointMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::Route>& route_,
    const std::shared_ptr<const GeoInfoDocument::RoutePoint>& routepoint_)
    : MapObject(geoInfoDocument_)
    , route(route_)
    , routepoint(routepoint_)
{
    const auto position31 = Utilities::convertLatLonTo31(routepoint->position);
    points31.push_back(position31);
    bbox31.topLeft = bbox31.bottomRight = position31;

    if (!routepoint->name.isEmpty())
    {
        captionsOrder.push_back(defaultEncodingDecodingRules->name_encodingRuleId);
        captions[defaultEncodingDecodingRules->name_encodingRuleId] = routepoint->name;
    }

    typesRuleIds.append(defaultEncodingDecodingRules->routepoint_encodingRuleId);
}

OsmAnd::GeoInfoPresenter::RoutepointMapObject::~RoutepointMapObject()
{
}

OsmAnd::GeoInfoPresenter::RoutelineMapObject::RoutelineMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::Route>& route_)
    : MapObject(geoInfoDocument_)
    , route(route_)
{
    if (!route->points.isEmpty())
    {
        points31.resize(route->points.size());
        auto pPosition31 = points31.data();
        for (const auto& routepoint : constOf(route->points))
            *(pPosition31++) = Utilities::convertLatLonTo31(routepoint->position);
        computeBBox31();
    }

    if (!route->name.isEmpty())
    {
        captionsOrder.push_back(defaultEncodingDecodingRules->name_encodingRuleId);
        captions[defaultEncodingDecodingRules->name_encodingRuleId] = route->name;
    }

    typesRuleIds.append(defaultEncodingDecodingRules->routeline_encodingRuleId);
}

OsmAnd::GeoInfoPresenter::RoutelineMapObject::~RoutelineMapObject()
{
}
