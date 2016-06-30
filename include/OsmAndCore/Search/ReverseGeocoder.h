#ifndef REVERSEGEOCODER_H
#define REVERSEGEOCODER_H

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QHash>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Nullable.h>

#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/Search/BaseSearch.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/IRoadLocator.h>


namespace OsmAnd
{
    class OSMAND_CORE_API ReverseGeocoder Q_DECL_FINAL : public BaseSearch
    {
        Q_DISABLE_COPY_AND_MOVE(ReverseGeocoder)
    public:
        const std::shared_ptr<const IRoadLocator> roadLocator;

        struct OSMAND_CORE_API Criteria : public BaseSearch::Criteria
        {
            Nullable<LatLon> latLon;
            Nullable<PointI> position31;
        };

        struct OSMAND_CORE_API ResultEntry : public IResultEntry
        {
            ResultEntry(const QString address);
            QString address;
        };

        ReverseGeocoder(
                const std::shared_ptr<const IObfsCollection>& obfsCollection,
                const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator);
        virtual ~ReverseGeocoder();

        virtual void performSearch(
            const ISearch::Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
            const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
    };
}

#endif // REVERSEGEOCODER_H
