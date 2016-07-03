#include "ReverseGeocoder.h"
#include "ReverseGeocoder_P.h"

#include "AddressesByNameSearch.h"
#include "Utilities.h"

const float DISTANCE_STREET_NAME_PROXIMITY_BY_NAME = 15000;

OsmAnd::ReverseGeocoder::ReverseGeocoder(
        const std::shared_ptr<const OsmAnd::IObfsCollection> &obfsCollection,
        const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator_)
    : BaseSearch(obfsCollection)
    , _p(new ReverseGeocoder_P(this, roadLocator_))
{

}

OsmAnd::ReverseGeocoder::~ReverseGeocoder()
{

}

void OsmAnd::ReverseGeocoder::performSearch(
    const ISearch::Criteria& criteria_,
    const NewResultEntryCallback newResultEntryCallback,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    return _p->performSearch(criteria_, newResultEntryCallback, queryController);
}

OsmAnd::AreaI OsmAnd::ReverseGeocoder::ResultEntry::searchBbox() const
{
    LatLon topLeft = LatLon(
                searchPoint->latitude - DISTANCE_STREET_NAME_PROXIMITY_BY_NAME,
                searchPoint->longitude - DISTANCE_STREET_NAME_PROXIMITY_BY_NAME);
    LatLon bottomRight = LatLon(
                searchPoint->latitude + DISTANCE_STREET_NAME_PROXIMITY_BY_NAME,
                searchPoint->longitude + DISTANCE_STREET_NAME_PROXIMITY_BY_NAME);
    return AreaI(Utilities::convertLatLonTo31(topLeft), Utilities::convertLatLonTo31(bottomRight));
}

OsmAnd::Nullable<OsmAnd::PointI> OsmAnd::ReverseGeocoder::ResultEntry::searchPoint31() const
{
    return searchPoint.isSet() ? Nullable<PointI>(Utilities::convertLatLonTo31(*searchPoint)) : Nullable<PointI>();
}

OsmAnd::ReverseGeocoder::ResultEntry::ResultEntry()
{

}

double OsmAnd::ReverseGeocoder::ResultEntry::getDistance() const
{
    return isnan(dist) ? Utilities::distance(connectionPoint, searchPoint) : dist;
}

void OsmAnd::ReverseGeocoder::ResultEntry::setDistance(double dist)
{
    this->dist = dist;
}

QString OsmAnd::ReverseGeocoder::ResultEntry::toString() const
{
    return QString() %
            (building ?              building->toString() : QString()) %
            (street ?                QStringLiteral(" ") % street->toString() : QString()) %
            (!streetName.isEmpty() ? QStringLiteral(" str. ") % streetName : QString()) %
            (streetGroup ?           QStringLiteral(" ") % streetGroup->toString() : QString()) %
            (!isnan(getDistance()) ? QStringLiteral(" dist=") % QString::number(getDistance()) : QString());
}
