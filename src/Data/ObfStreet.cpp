#include "ObfStreet.h"
#include "StreetGroup.h"

OsmAnd::ObfStreet::ObfStreet(const std::shared_ptr<const OsmAnd::ObfStreetGroup> &streetGroup,
        std::shared_ptr<const Street> street,
        OsmAnd::ObfObjectId id,
        uint32_t offset,
        uint32_t firstBuildingInnerOffset,
        uint32_t firstIntersectionInnerOffset)
    : ObfAddress(AddressType::Street, id)
    , _obfStreetGroup(streetGroup)
	, _street(street)
    , _offset(offset)
    , _firstBuildingInnerOffset(firstBuildingInnerOffset)
    , _firstIntersectionInnerOffset(firstIntersectionInnerOffset)
{
    assert(street);
}

std::shared_ptr<const OsmAnd::ObfStreetGroup> OsmAnd::ObfStreet::obfStreetGroup() const
{
    return _obfStreetGroup;
}

OsmAnd::StreetGroup OsmAnd::ObfStreet::streetGroup() const
{
    return _obfStreetGroup->streetGroup();
}

OsmAnd::Street OsmAnd::ObfStreet::street() const
{
    return *_street;
}

std::shared_ptr<const OsmAnd::Street> OsmAnd::ObfStreet::streetPtr() const
{
    return _street;
}

uint32_t OsmAnd::ObfStreet::offset() const
{
    return _offset;
}

uint32_t OsmAnd::ObfStreet::firstBuildingInnerOffset() const
{
    return _firstBuildingInnerOffset;
}

uint32_t OsmAnd::ObfStreet::firstIntersectionInnerOffset() const
{
    return _firstIntersectionInnerOffset;
}
