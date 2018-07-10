#ifndef _OSMAND_CORE_TRANSPORT_STOPS_IN_AREA_SEARCH_H_
#define _OSMAND_CORE_TRANSPORT_STOPS_IN_AREA_SEARCH_H_

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
    class TransportStop;

    class OSMAND_CORE_API TransportStopsInAreaSearch Q_DECL_FINAL : public BaseSearch
    {
        Q_DISABLE_COPY_AND_MOVE(TransportStopsInAreaSearch);
    public:        
        static const int TRANSPORT_STOP_ZOOM = 24;

        struct OSMAND_CORE_API Criteria : public BaseSearch::Criteria
        {
            Criteria();
            virtual ~Criteria();

            Nullable<AreaI> bbox31;
            Nullable<AreaI> obfInfoAreaFilter;
        };

        struct OSMAND_CORE_API ResultEntry : public IResultEntry
        {
            ResultEntry();
            virtual ~ResultEntry();

            std::shared_ptr<const TransportStop> transportStop;
        };

    private:
    protected:
    public:
        TransportStopsInAreaSearch(const std::shared_ptr<const IObfsCollection>& obfsCollection);
        virtual ~TransportStopsInAreaSearch();

        virtual void performSearch(
            const ISearch::Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
    };
}

#endif // !defined(_OSMAND_CORE_TRANSPORT_STOPS_IN_AREA_SEARCH_H_)
