#include "ReverseGeocoder.h"
#include "ReverseGeocoder_P.h"

#include "AddressesByNameSearch.h"
#include "Utilities.h"

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
    return _p->performSearch(criteria_, nullptr, newResultEntryCallback, queryController);
}

std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> OsmAnd::ReverseGeocoder::performSearch(
    const Criteria& criteria, const std::shared_ptr<RoutingContext>& ctx) const
{
    std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> result;
    _p->performSearch(
                criteria,
                ctx,
                [&result](const OsmAnd::ISearch::Criteria& criteria, const OsmAnd::BaseSearch::IResultEntry& resultEntry) {
                    result = std::make_shared<const OsmAnd::ReverseGeocoder::ResultEntry>(static_cast<const OsmAnd::ReverseGeocoder::ResultEntry&>(resultEntry));
    });
    return result;
}

OsmAnd::ReverseGeocoder::ResultEntry::ResultEntry()
{
}

OsmAnd::ReverseGeocoder::ResultEntry::~ResultEntry()
{
}

OsmAnd::Nullable<OsmAnd::PointI> OsmAnd::ReverseGeocoder::ResultEntry::searchPoint31() const
{
    return searchPoint.isSet() ? Nullable<PointI>(Utilities::convertLatLonTo31(*searchPoint)) : Nullable<PointI>();
}

double OsmAnd::ReverseGeocoder::ResultEntry::getSortDistance() const
{
    double dist = getDistance();
    if (dist > 0 && building == nullptr)
    {
        // add extra distance to match buildings first
        return dist + 50;
    }
    return dist;
}

double OsmAnd::ReverseGeocoder::ResultEntry::getDistance() const
{
    if ((std::isnan(dist) || dist == -1) && searchPoint.isSet())
    {
        if (building == nullptr && point != nullptr)
        {
            // Need distance between searchPoint and nearest RouteSegmentPoint here, to approximate distance from neareest named road
            dist = point->dist;
        }
        else if (connectionPoint.isSet() && searchPoint.isSet())
        {
            dist = Utilities::distance(connectionPoint, searchPoint);
        }
    }
    return dist;
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
            ((!std::isnan(getDistance()) && getDistance() > 0) ? QStringLiteral(" dist=") % QString::number(getDistance()) : QString());
}

OsmAnd::ReverseGeocoder::Criteria::Criteria()
{
}

OsmAnd::ReverseGeocoder::Criteria::~Criteria()
{
}

