#include "GeoInfoPresenter_P.h"
#include "GeoInfoPresenter.h"

#include "GeoInfoDocument.h"
#include "MapObjectsProvider.h"

OsmAnd::GeoInfoPresenter_P::GeoInfoPresenter_P(GeoInfoPresenter* const owner_)
    : owner(owner_)
{
}

OsmAnd::GeoInfoPresenter_P::~GeoInfoPresenter_P()
{
}

QList< std::shared_ptr<const OsmAnd::GeoInfoPresenter_P::MapObject> > OsmAnd::GeoInfoPresenter_P::generateMapObjects(
    const QList< std::shared_ptr<const GeoInfoDocument> >& geoInfoDocuments)
{
    QList< std::shared_ptr<const MapObject> > mapObjects;

    for (const auto& geoInfoDocument : constOf(geoInfoDocuments))
    {
        for (const auto& waypoint : constOf(geoInfoDocument->points))
        {
            const std::shared_ptr<WaypointMapObject> newMapObject(new WaypointMapObject(
                geoInfoDocument,
                waypoint));
            mapObjects.append(newMapObject);
        }

        for (const auto& track : constOf(geoInfoDocument->tracks))
        {
            for (const auto& trackSegment : constOf(track->segments))
            {
                if (trackSegment->points.isEmpty())
                    continue;

                const std::shared_ptr<TracklineMapObject> newMapObject(new TracklineMapObject(
                    geoInfoDocument,
                    track,
                    trackSegment));
                mapObjects.append(newMapObject);

                for (const auto& trackpoint : constOf(trackSegment->points))
                {
                    const std::shared_ptr<TrackpointMapObject> newMapObject(new TrackpointMapObject(
                        geoInfoDocument,
                        track,
                        trackSegment,
                        trackpoint));
                    mapObjects.append(newMapObject);
                }
            }
        }

        for (const auto& route : constOf(geoInfoDocument->routes))
        {
            if (route->points.isEmpty())
                continue;

            const std::shared_ptr<RoutelineMapObject> newMapObject(new RoutelineMapObject(
                geoInfoDocument,
                route));
            mapObjects.append(newMapObject);

            for (const auto& routepoint : constOf(route->points))
            {
                const std::shared_ptr<RoutepointMapObject> newMapObject(new RoutepointMapObject(
                    geoInfoDocument,
                    route,
                    routepoint));
                mapObjects.append(newMapObject);
            }
        }
    }

    return mapObjects;
}

std::shared_ptr<OsmAnd::IMapObjectsProvider> OsmAnd::GeoInfoPresenter_P::createMapObjectsProvider() const
{
    const auto mapObjects = generateMapObjects(owner->documents);
    return std::shared_ptr<MapObjectsProvider>(new MapObjectsProvider(
        copyAs< QList< std::shared_ptr<const OsmAnd::MapObject> > >(mapObjects)));
}
