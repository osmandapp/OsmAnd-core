#ifndef _OSMAND_CORE_BUILDING_H_
#define _OSMAND_CORE_BUILDING_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/MemoryCommon.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Data/Address.h>
#include <OsmAndCore/Data/DataCommonTypes.h>

namespace OsmAnd
{
    class Street;
    class StreetGroup;

    class OSMAND_CORE_API Building Q_DECL_FINAL: public Address
    {
        Q_DISABLE_COPY_AND_MOVE(Building);

    public:
        enum class Interpolation : int32_t
        {
            Disabled = 0,
            All = -1,
            Even = -2,
            Odd = -3,
            Alphabetic = -4
        };

    private:
    protected:
    public:
        Building(const std::shared_ptr<const Street>& street);
        Building(const std::shared_ptr<const StreetGroup>& streetGroup);
        virtual ~Building();
        virtual QString toString() const;

        const std::shared_ptr<const Street> street;
        const std::shared_ptr<const StreetGroup> streetGroup;

        QString postcode;

        Interpolation interpolation;
        int32_t interpolationInterval;
        QString interpolationNativeName;
        QHash<QString, QString> interpolationLocalizedNames;
        PointI interpolationPosition31;
    };
}

#endif // !defined(_OSMAND_CORE_BUILDING_H_)
