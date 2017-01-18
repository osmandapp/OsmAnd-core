#include "Street.h"

#include "StreetGroup.h"

#include <QStringBuilder>

OsmAnd::Street::Street(const std::shared_ptr<const StreetGroup>& streetGroup_)
    : Address(streetGroup_->obfSection, AddressType::Street)
    , streetGroup(streetGroup_)
    , offset(0)
    , firstBuildingInnerOffset(0)
    , firstIntersectionInnerOffset(0)
{
}

OsmAnd::Street::~Street()
{
}

QString OsmAnd::Street::toString() const
{
    return QStringLiteral("str. ") % nativeName;
}
