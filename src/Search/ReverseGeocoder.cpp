#include "ReverseGeocoder.h"
#include "Utilities.h"

const float THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER = 4;
const float STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS = 100;
const float STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS = 400;

const float DISTANCE_STREET_NAME_PROXIMITY_BY_NAME = 15000;
const float DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME = 1000;

const float THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER = 1.5f;
const float DISTANCE_BUILDING_PROXIMITY = 100;


QVector<OsmAnd::GeocodingResult> OsmAnd::ReverseGeocoder::reverseGeocodingSearch(const RoadLocator &roadLocator, LatLon latLon)
{
    QVector<GeocodingResult> result{};
    auto position31 = Utilities::convertLatLonTo31(latLon);
    roadLocator.findNearestRoads(position31, STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS);
    double distSquare = 0;
//    QSet< set = new TLongHashSet();
//    Set<String> streetNames = new HashSet<String>();
//    for (RouteSegmentPoint p : listR) {
//        RouteDataObject road = p.getRoad();
//        if (!set.add(road.getId())) {
//            continue;
//        }
////			System.out.println(road.toString() +  " " + Math.sqrt(p.distSquare));
//        boolean emptyName = Algorithms.isEmpty(road.getName()) && Algorithms.isEmpty(road.getRef());
//        if (!emptyName) {
//            if (distSquare == 0 || distSquare > p.distSquare) {
//                distSquare = p.distSquare;
//            }
//            GeocodingResult sr = new GeocodingResult();
//            sr.searchPoint = new LatLon(lat, lon);
//            sr.streetName = Algorithms.isEmpty(road.getName()) ? road.getRef() : road.getName();
//            sr.point = p;
//            sr.connectionPoint = new LatLon(MapUtils.get31LatitudeY(p.preciseY), MapUtils.get31LongitudeX(p.preciseX));
//            sr.regionFP = road.region.getFilePointer();
//            sr.regionLen = road.region.getLength();
//            if (streetNames.add(sr.streetName)) {
//                lst.add(sr);
//            }
//        }
//        if (p.distSquare > STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS * STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS &&
//                distSquare != 0 && p.distSquare > THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER * distSquare) {
//            break;
//        }
//        if (p.distSquare > STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS * STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS) {
//            break;
//        }
//    }
//    Collections.sort(lst, GeocodingUtilities.DISTANCE_COMPARATOR);
//    return lst;
}
