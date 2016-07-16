#ifndef _OSMAND_CORE_OBF_STREET_INTERSECTION_H_
#define _OSMAND_CORE_OBF_STREET_INTERSECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfAddress.h>
#include <OsmAndCore/Data/ObfStreet.h>
#include <OsmAndCore/Data/StreetIntersection.h>

namespace OsmAnd
{
    class ObfStreet;

    class OSMAND_CORE_API ObfStreetIntersection Q_DECL_FINAL : public ObfAddress
    {
    public:
        ObfStreetIntersection(
                StreetIntersection streetIntersection,
                std::shared_ptr<const ObfStreet> street);

        StreetIntersection streetIntersection() const;
        std::shared_ptr<const ObfStreet> street() const;

    private:
        StreetIntersection _streetIntersection;
        std::shared_ptr<const ObfStreet> _street;
    };
}

#endif // !defined(_OSMAND_CORE_OBF_STREET_INTERSECTION_H_)
