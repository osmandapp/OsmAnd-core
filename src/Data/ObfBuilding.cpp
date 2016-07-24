#include "Building.h"
#include "ObfAddress.h"
#include "ObfBuilding.h"
#include "ObfStreet.h"
#include "ObfStreetGroup.h"

OsmAnd::ObfBuilding::ObfBuilding(
        OsmAnd::Building building,
        OsmAnd::ObfObjectId id,
        std::shared_ptr<const ObfStreet> street)
    : ObfAddress(AddressType::Building, id)
    , _building(building)
    , _street(street)
    , _streetGroup(street->obfStreetGroup())
{

}

OsmAnd::ObfBuilding::ObfBuilding(
        OsmAnd::Building building,
        OsmAnd::ObfObjectId id,
        std::shared_ptr<const OsmAnd::ObfStreetGroup> streetGroup)
    : ObfAddress(AddressType::Building, id)
    , _building(building)
    , _street(nullptr)
    , _streetGroup(streetGroup)
{

}

OsmAnd::Building OsmAnd::ObfBuilding::building() const
{
    return _building;
}

std::shared_ptr<const OsmAnd::ObfStreet> OsmAnd::ObfBuilding::street() const
{
    return _street;
}

std::shared_ptr<const OsmAnd::ObfStreetGroup> OsmAnd::ObfBuilding::streetGroup() const
{
    return _streetGroup;
}
