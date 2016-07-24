#include "ObfStreetIntersection.h"

#include "ObfAddress.h"
#include "ObfStreet.h"
#include "StreetIntersection.h"

OsmAnd::ObfStreetIntersection::ObfStreetIntersection(
        OsmAnd::StreetIntersection streetIntersection,
        std::shared_ptr<const OsmAnd::ObfStreet> street)
    : ObfAddress(AddressType::StreetIntersection)
    , _streetIntersection(streetIntersection)
    , _street(street)
{

}

OsmAnd::StreetIntersection OsmAnd::ObfStreetIntersection::streetIntersection() const
{
    return _streetIntersection;
}

std::shared_ptr<const OsmAnd::ObfStreet> OsmAnd::ObfStreetIntersection::street() const
{
    return _street;
}
