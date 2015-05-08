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

std::shared_ptr<const OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping>
OsmAnd::GeoInfoPresenter::MapObject::defaultAttributeMapping(OsmAnd::modifyAndReturn(
    std::shared_ptr<OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping>(new OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping()),
    static_cast< std::function<void(std::shared_ptr<OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping>& instance)> >([]
    (std::shared_ptr<OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping>& rules) -> void
    {
        rules->verifyRequiredMappingRegistered();
    })));

OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping::AttributeMapping()
    : waypointAttributeId(std::numeric_limits<uint32_t>::max())
    , trackpointAttributeId(std::numeric_limits<uint32_t>::max())
    , routepointAttributeId(std::numeric_limits<uint32_t>::max())
    , tracklineAttributeId(std::numeric_limits<uint32_t>::max())
    , routelineAttributeId(std::numeric_limits<uint32_t>::max())
    , waypointsPresentAttributeId(std::numeric_limits<uint32_t>::max())
    , waypointsNotPresentAttributeId(std::numeric_limits<uint32_t>::max())
    , trackpointsPresentAttributeId(std::numeric_limits<uint32_t>::max())
    , trackpointsNotPresentAttributeId(std::numeric_limits<uint32_t>::max())
    , routepointsPresentAttributeId(std::numeric_limits<uint32_t>::max())
    , routepointsNotPresentAttributeId(std::numeric_limits<uint32_t>::max())
{
}

OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping::~AttributeMapping()
{
}

void OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping::registerRequiredMapping(uint32_t& lastUsedEntryId)
{
    if (waypointAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("osmand"), QLatin1String("file_waypoint"));
    }

    if (trackpointAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("osmand"), QLatin1String("file_trackpoint"));
    }

    if (routepointAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("osmand"), QLatin1String("file_routepoint"));
    }

    if (tracklineAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("osmand"), QLatin1String("file_trackline"));
    }

    if (routelineAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("osmand"), QLatin1String("file_routeline"));
    }

    if (routelineAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("osmand"), QLatin1String("file_routeline"));
    }

    if (waypointsPresentAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("file_has_waypoints"), QLatin1String("true"));
    }

    if (waypointsNotPresentAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("file_has_waypoints"), QLatin1String("false"));
    }

    if (trackpointsPresentAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("file_has_trackpoints"), QLatin1String("true"));
    }

    if (trackpointsNotPresentAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("file_has_trackpoints"), QLatin1String("false"));
    }

    if (routepointsPresentAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("file_has_routepoints"), QLatin1String("true"));
    }

    if (routepointsNotPresentAttributeId == std::numeric_limits<uint32_t>::max())
    {
        registerMapping(++lastUsedEntryId,
            QLatin1String("file_has_routepoints"), QLatin1String("false"));
    }
}

void OsmAnd::GeoInfoPresenter::MapObject::AttributeMapping::registerMapping(
    const uint32_t id,
    const QString& tag,
    const QString& value)
{
    OsmAnd::MapObject::AttributeMapping::registerMapping(id, tag, value);

    if (QLatin1String("osmand") == tag && QLatin1String("file_waypoint") == value)
        waypointAttributeId = id;
    else if (QLatin1String("osmand") == tag && QLatin1String("file_trackpoint") == value)
        trackpointAttributeId = id;
    else if (QLatin1String("osmand") == tag && QLatin1String("file_routepoint") == value)
        routepointAttributeId = id;
    else if (QLatin1String("osmand") == tag && QLatin1String("file_trackline") == value)
        tracklineAttributeId = id;
    else if (QLatin1String("osmand") == tag && QLatin1String("file_routeline") == value)
        routelineAttributeId = id;
    else if (QLatin1String("file_has_waypoints") == tag && QLatin1String("true") == value)
        waypointsPresentAttributeId = id;
    else if (QLatin1String("file_has_waypoints") == tag && QLatin1String("false") == value)
        waypointsNotPresentAttributeId = id;
    else if (QLatin1String("file_has_trackpoints") == tag && QLatin1String("true") == value)
        trackpointsPresentAttributeId = id;
    else if (QLatin1String("file_has_trackpoints") == tag && QLatin1String("false") == value)
        trackpointsNotPresentAttributeId = id;
    else if (QLatin1String("file_has_routepoints") == tag && QLatin1String("true") == value)
        routepointsPresentAttributeId = id;
    else if (QLatin1String("file_has_routepoints") == tag && QLatin1String("false") == value)
        routepointsNotPresentAttributeId = id;
}

OsmAnd::GeoInfoPresenter::MapObject::MapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_)
    : geoInfoDocument(geoInfoDocument_)
{
    attributeMapping = defaultAttributeMapping;

    additionalAttributeIds.append(geoInfoDocument->locationMarks.isEmpty()
        ? defaultAttributeMapping->waypointsNotPresentAttributeId
        : defaultAttributeMapping->waypointsPresentAttributeId);

    additionalAttributeIds.append(geoInfoDocument->tracks.isEmpty()
        ? defaultAttributeMapping->trackpointsNotPresentAttributeId
        : defaultAttributeMapping->trackpointsPresentAttributeId);

    additionalAttributeIds.append(geoInfoDocument->routes.isEmpty()
        ? defaultAttributeMapping->routepointsNotPresentAttributeId
        : defaultAttributeMapping->routepointsPresentAttributeId);
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
        captionsOrder.push_back(defaultAttributeMapping->nativeNameAttributeId);
        captions[defaultAttributeMapping->nativeNameAttributeId] = waypoint->name;
    }

    attributeIds.append(defaultAttributeMapping->waypointAttributeId);
}

OsmAnd::GeoInfoPresenter::WaypointMapObject::~WaypointMapObject()
{
}

OsmAnd::GeoInfoPresenter::TrackpointMapObject::TrackpointMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::Track>& track_,
    const std::shared_ptr<const GeoInfoDocument::TrackSegment>& trackSegment_,
    const std::shared_ptr<const GeoInfoDocument::LocationMark>& trackpoint_)
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
        captionsOrder.push_back(defaultAttributeMapping->nativeNameAttributeId);
        captions[defaultAttributeMapping->nativeNameAttributeId] = trackpoint->name;
    }

    attributeIds.append(defaultAttributeMapping->trackpointAttributeId);
}

OsmAnd::GeoInfoPresenter::TrackpointMapObject::~TrackpointMapObject()
{
}

OsmAnd::GeoInfoPresenter::TracklineMapObject::TracklineMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::Track>& track_,
    const std::shared_ptr<const GeoInfoDocument::TrackSegment>& trackSegment_)
    : MapObject(geoInfoDocument_)
    , track(track_)
    , trackSegment(trackSegment_)
{
    points31.resize(trackSegment->points.size());
    auto pPosition31 = points31.data();
    for (const auto& trackpoint : constOf(trackSegment->points))
        *(pPosition31++) = Utilities::convertLatLonTo31(trackpoint->position);
    computeBBox31();
    
    if (!track->name.isEmpty())
    {
        captionsOrder.push_back(defaultAttributeMapping->nativeNameAttributeId);
        captions[defaultAttributeMapping->nativeNameAttributeId] = track->name;
    }

    attributeIds.append(defaultAttributeMapping->tracklineAttributeId);
}

OsmAnd::GeoInfoPresenter::TracklineMapObject::~TracklineMapObject()
{
}

OsmAnd::GeoInfoPresenter::RoutepointMapObject::RoutepointMapObject(
    const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument_,
    const std::shared_ptr<const GeoInfoDocument::Route>& route_,
    const std::shared_ptr<const GeoInfoDocument::LocationMark   >& routepoint_)
    : MapObject(geoInfoDocument_)
    , route(route_)
    , routepoint(routepoint_)
{
    const auto position31 = Utilities::convertLatLonTo31(routepoint->position);
    points31.push_back(position31);
    bbox31.topLeft = bbox31.bottomRight = position31;

    if (!routepoint->name.isEmpty())
    {
        captionsOrder.push_back(defaultAttributeMapping->nativeNameAttributeId);
        captions[defaultAttributeMapping->nativeNameAttributeId] = routepoint->name;
    }

    attributeIds.append(defaultAttributeMapping->routepointAttributeId);
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
    points31.resize(route->points.size());
    auto pPosition31 = points31.data();
    for (const auto& routepoint : constOf(route->points))
        *(pPosition31++) = Utilities::convertLatLonTo31(routepoint->position);
    computeBBox31();

    if (!route->name.isEmpty())
    {
        captionsOrder.push_back(defaultAttributeMapping->nativeNameAttributeId);
        captions[defaultAttributeMapping->nativeNameAttributeId] = route->name;
    }

    attributeIds.append(defaultAttributeMapping->routelineAttributeId);
}

OsmAnd::GeoInfoPresenter::RoutelineMapObject::~RoutelineMapObject()
{
}
