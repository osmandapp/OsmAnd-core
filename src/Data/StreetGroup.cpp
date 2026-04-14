#include "StreetGroup.h"

#include <QStringBuilder>

OsmAnd::StreetGroup::StreetGroup(const std::shared_ptr<const ObfAddressSectionInfo>& obfSection_)
    : Address(obfSection_, AddressType::StreetGroup)
    , dataOffset(0)
{
}

OsmAnd::StreetGroup::~StreetGroup()
{
}

QString OsmAnd::StreetGroup::toString() const
{
    return QStringLiteral("city ") % nativeName;
}
