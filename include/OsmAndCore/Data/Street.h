#ifndef _OSMAND_CORE_STREET_H_
#define _OSMAND_CORE_STREET_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Data/Address.h>

namespace OsmAnd
{
    class StreetGroup;

    class OSMAND_CORE_API Street Q_DECL_FINAL : public Address
    {
    public:
        virtual QString toString() const;
    };
}

#endif // !defined(_OSMAND_CORE_STREET_H_)

