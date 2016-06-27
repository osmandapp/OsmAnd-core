#ifndef _OSMAND_CORE_STREET_INTERSECTION_H_
#define _OSMAND_CORE_STREET_INTERSECTION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/Address.h>

namespace OsmAnd
{
    class Street;

    class OSMAND_CORE_API StreetIntersection Q_DECL_FINAL : public Address
    {
        Q_DISABLE_COPY_AND_MOVE(StreetIntersection);

    private:
    protected:
    public:
        StreetIntersection(const std::shared_ptr<const Street>& street);
        virtual ~StreetIntersection();

        const std::shared_ptr<const Street> street;

        QString nativeName;
        QHash<QString, QString> localizedNames;
        PointI position31;
    };
}

#endif // !defined(_OSMAND_CORE_STREET_INTERSECTION_H_)
