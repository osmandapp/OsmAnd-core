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

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "IRoadLocator.h"
#include "LatLon.h"
#include "AddressesByNameSearch.h"
#include "ISearch.h"
#include "ReverseGeocoder.h"

namespace OsmAnd
{
    class ReverseGeocoder_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(ReverseGeocoder_P)

        using ResultEntry = ReverseGeocoder::ResultEntry;
        using Criteria = ReverseGeocoder::Criteria;

    private:
        const std::shared_ptr<const IRoadLocator> roadLocator;
        const std::shared_ptr<const AddressesByNameSearch> addressByNameSearch;

        static bool DISTANCE_COMPARATOR(
                const std::shared_ptr<const ResultEntry> &a,
                const std::shared_ptr<const ResultEntry> &b);

        std::shared_ptr<const ResultEntry> justifyResult(
                QVector<std::shared_ptr<const ResultEntry>> res) const;
        QVector<std::shared_ptr<const ResultEntry>> justifyReverseGeocodingSearch(
                const std::shared_ptr<const ResultEntry> &road,
                double knownMinBuildingDistance) const;
        QVector<std::shared_ptr<const ResultEntry>> loadStreetBuildings(
                const std::shared_ptr<const ResultEntry> road,
                const std::shared_ptr<const ResultEntry> street) const;
        QVector<std::shared_ptr<const ResultEntry>> reverseGeocodeToRoads(
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
