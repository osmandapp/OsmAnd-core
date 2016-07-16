#ifndef _OSMAND_CORE_OBF_ADDRESS_H_
#define _OSMAND_CORE_OBF_ADDRESS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>
#include <QMap>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/Address.h>

namespace OsmAnd
{
    class ObfAddressSectionInfo;
    class Address;

    class OSMAND_CORE_API ObfAddress
    {
    public:
        AddressType type() const;
        ObfObjectId id() const;

    protected:
        ObfAddress(
                AddressType type,
                ObfObjectId id = ObfObjectId::invalidId());

        ObfObjectId _id = ObfObjectId::invalidId();
        AddressType _type;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_ADDRESS_H_)
