#ifndef _OSMAND_CORE_REVERSE_GEOCODER_P_H_
#define _OSMAND_CORE_REVERSE_GEOCODER_P_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QHash>
#include <QList>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/Data/Street.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/IRoadLocator.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Search/AddressesByNameSearch.h>
#include <OsmAndCore/Search/BaseSearch.h>
#include <OsmAndCore/Search/ISearch.h>
#include <OsmAndCore/Search/ReverseGeocoder.h>

namespace OsmAnd
{
    class ReverseGeocoder_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ReverseGeocoder_P)
    private:
        const std::shared_ptr<const IRoadLocator> roadLocator;
        AddressesByNameSearch* addressByNameSearch;

        static bool DISTANCE_COMPARATOR(
                const std::shared_ptr<const ReverseGeocoder::ResultEntry> &a,
                const std::shared_ptr<const ReverseGeocoder::ResultEntry> &b);

        std::shared_ptr<const ReverseGeocoder::ResultEntry> justifyResult(
                QVector<std::shared_ptr<const ReverseGeocoder::ResultEntry>> res) const;
        QVector<std::shared_ptr<const ReverseGeocoder::ResultEntry>> justifyReverseGeocodingSearch(
                const std::shared_ptr<const ReverseGeocoder::ResultEntry> &road,
                double knownMinBuildingDistance) const;
        QVector<std::shared_ptr<const ReverseGeocoder::ResultEntry>> loadStreetBuildings(
                const std::shared_ptr<const ReverseGeocoder::ResultEntry> road,
                const std::shared_ptr<const ReverseGeocoder::ResultEntry> street) const;
        QVector<std::shared_ptr<const ReverseGeocoder::ResultEntry>> reverseGeocodeToRoads(
                const LatLon searchPoint) const;
    protected:
        ImplementationInterface<ReverseGeocoder> owner;
    public:
        explicit ReverseGeocoder_P(
                ReverseGeocoder* owner_,
                const std::shared_ptr<const IRoadLocator>& roadLocator);
        virtual ~ReverseGeocoder_P();

        virtual void performSearch(
                const ISearch::Criteria& criteria,
                const ISearch::NewResultEntryCallback newResultEntryCallback,
                const std::shared_ptr<const IQueryController>& queryController = nullptr) const;

        friend class OsmAnd::ReverseGeocoder;
    };
}

#endif // _OSMAND_CORE_REVERSE_GEOCODER_P_H_
