#include "ObfAddress.h"

OsmAnd::ObfAddress::ObfAddress(
        OsmAnd::AddressType type,
        OsmAnd::ObfObjectId id)
    : _type(type)
    , _id(id)
{

}

OsmAnd::ObfObjectId OsmAnd::ObfAddress::id() const
{
    return _id;
}

OsmAnd::AddressType OsmAnd::ObfAddress::type() const
{
    return _type;
}
