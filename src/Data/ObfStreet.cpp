#include "ObfStreet.h"
#include "StreetGroup.h"

OsmAnd::ObfStreet::ObfStreet(
        const std::shared_ptr<const OsmAnd::ObfStreetGroup> &streetGroup_,
        OsmAnd::Street street_,
        OsmAnd::ObfObjectId id_,
        uint32_t offset_,
        uint32_t firstBuildingInnerOffset_,
        uint32_t firstIntersectionInnerOffset_)
    : obfStreetGroup(streetGroup_)
	, street(street_)
    , id(id_)
    , offset(offset_)
    , firstBuildingInnerOffset(firstBuildingInnerOffset_)
    , firstIntersectionInnerOffset(firstIntersectionInnerOffset_)
{

}
