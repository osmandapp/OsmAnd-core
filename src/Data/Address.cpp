#include "Address.h"

OsmAnd::Address::Address(
        OsmAnd::AddressType addressType_,
        QString nativeName_,
        QHash<QString, QString> localizedNames_,
        OsmAnd::PointI position31_)
    : addressType(addressType_)
    , nativeName(nativeName_)
    , localizedNames(localizedNames_)
    , position31(position31_)
{

}

QString OsmAnd::Address::toString() const
{
    return ADDRESS_TYPE_NAMES[addressType];
}
