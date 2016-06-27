#include "StreetIntersection.h"
#include "Street.h"

OsmAnd::StreetIntersection::StreetIntersection(const std::shared_ptr<const Street>& street_)
    : Address(street_->obfSection, AddressType::StreetIntersection)
    , street(street_)
{
}

OsmAnd::StreetIntersection::~StreetIntersection()
{
}
