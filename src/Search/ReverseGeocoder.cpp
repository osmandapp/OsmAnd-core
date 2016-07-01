#include "ReverseGeocoder.h"

#include "AddressesByNameSearch.h"
#include "Road.h"
#include "Utilities.h"

const float THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER = 4;
const float STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS = 100;
const float STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS = 400;

const float DISTANCE_STREET_NAME_PROXIMITY_BY_NAME = 15000;
const float DISTANCE_STREET_FROM_CLOSEST_WITH_SAME_NAME = 1000;

const float THRESHOLD_MULTIPLIER_SKIP_BUILDINGS_AFTER = 1.5f;
const float DISTANCE_BUILDING_PROXIMITY = 100;


OsmAnd::ReverseGeocoder::ReverseGeocoder(
        const std::shared_ptr<const OsmAnd::IObfsCollection> &obfsCollection,
        const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator_)
    : BaseSearch(obfsCollection), roadLocator(roadLocator_)
{

}

void OsmAnd::ReverseGeocoder::performSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    const auto criteria = *dynamic_cast<const Criteria*>(&criteria_);
    if (!criteria.latLon.isSet() && !criteria.position31.isSet())
        return;
    auto position31 = criteria.position31.isSet() ? *criteria.position31 : Utilities::convertLatLonTo31(*criteria.latLon);
    auto roads = roadLocator->findNearestRoads(position31, std::pow(STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS, 2));
    double distSquare = 0;
    QSet<ObfObjectId> set{};
    for (auto p : roads)
    {
        const Road& road = *p.first;
        double roadDistSquare = p.second;
        if (set.contains(road.id))
            continue;
        else
            set.insert(road.id);
        if (!road.captions.isEmpty())
        {
            if (distSquare == 0 || distSquare > roadDistSquare)
                distSquare = roadDistSquare;
            ResultEntry result{road.getCaptionInNativeLanguage() % " (" % QString::number(std::sqrt(roadDistSquare)) % " m)"};
            newResultEntryCallback(criteria_, result);
        }
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITH_MULTIPLIER_RADIUS, 2) &&
                distSquare != 0 && roadDistSquare > THRESHOLD_MULTIPLIER_SKIP_STREETS_AFTER * distSquare)
            break;
        if (roadDistSquare > std::pow(STOP_SEARCHING_STREET_WITHOUT_MULTIPLIER_RADIUS, 2))
            break;
    }
}

OsmAnd::ReverseGeocoder::ResultEntry::ResultEntry(const QString address)
{
    this->address = address;
}
