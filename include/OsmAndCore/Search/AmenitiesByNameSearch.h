#ifndef _OSMAND_CORE_AMENITIES_BY_NAME_SEARCH_H_
#define _OSMAND_CORE_AMENITIES_BY_NAME_SEARCH_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QHash>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Nullable.h>

#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Search/BaseSearch.h>
#include <OsmAndCore/ResourcesManager.h>

namespace OsmAnd
{
    class Amenity;

    class OSMAND_CORE_API AmenitiesByNameSearch Q_DECL_FINAL : public BaseSearch
    {
        Q_DISABLE_COPY_AND_MOVE(AmenitiesByNameSearch);
    public:
        struct OSMAND_CORE_API Criteria : public BaseSearch::Criteria
        {
            Criteria();
            virtual ~Criteria();

            Nullable<PointI> xy31;
            Nullable<AreaI> bbox31;
            Nullable<AreaI> obfInfoAreaFilter;
            TileAcceptorFunction tileFilter;
            QString name;
            QHash<QString, QStringList> categoriesFilter;
            QList< std::shared_ptr<const ResourcesManager::LocalResource> > localResources;
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
        AmenitiesByNameSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~AmenitiesByNameSearch();

        virtual void performSearch(
            const ISearch::Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
        
        virtual void performTravelGuidesSearch(
            const ISearch::Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_AMENITIES_BY_NAME_SEARCH_H_)
