#ifndef _OSMAND_CORE_AMENITIES_IN_AREA_SEARCH_H_
#define _OSMAND_CORE_AMENITIES_IN_AREA_SEARCH_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QHash>

#include <OsmAndCore.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Search/BaseSearch.h>

namespace OsmAnd
{
    class Amenity;

    class OSMAND_CORE_API AmenitiesInAreaSearch Q_DECL_FINAL : public BaseSearch
    {
        Q_DISABLE_COPY_AND_MOVE(AmenitiesInAreaSearch);
    public:
        struct OSMAND_CORE_API Criteria : public BaseSearch::Criteria
        {
            Criteria();
            virtual ~Criteria();

            QHash<QString, QStringList> categoriesFilter;
        };

        struct OSMAND_CORE_API ResultEntry : public IResultEntry
        {
            ResultEntry();
            virtual ~ResultEntry();

            std::shared_ptr<const Amenity> amenity;
        };

    private:
    protected:
    public:
        AmenitiesInAreaSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~AmenitiesInAreaSearch();

        virtual void performSearch(
            const ISearch::Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const IQueryController* const controller = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_AMENITIES_IN_AREA_SEARCH_H_)
