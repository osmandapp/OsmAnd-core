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

        struct StreetsCache;

        QVector<std::shared_ptr<const ResultEntry>> reverseGeocodingSearch(
                const LatLon searchPoint) const;
        QVector<std::shared_ptr<const ResultEntry>> findAddresses(
                const QVector<std::shared_ptr<const ResultEntry>>& roads,
                const std::shared_ptr<const IQueryController>& queryController) const;
        QVector<std::shared_ptr<const ResultEntry>> findStreetAndBuildings(
                const std::shared_ptr<const ResultEntry>& road,
                double knownMinBuildingDistance,
                StreetsCache& cache,
                const std::shared_ptr<const IQueryController>& queryController) const;
        bool addStreet(
                const std::shared_ptr<const ResultEntry>& road,
                const std::shared_ptr<const Street>& street,
                bool matchWithCommonWords,
                QVector<std::shared_ptr<ResultEntry>>& streetsList,
                StreetsCache& cache) const;
        QVector<std::shared_ptr<const ResultEntry>> loadStreetBuildings(
                const std::shared_ptr<const ResultEntry>& road,
                const std::shared_ptr<const ResultEntry>& street,
                StreetsCache& cache,
                const std::shared_ptr<const IQueryController>& queryController) const;
        static void filterDuplicateRegionResults(QVector<std::shared_ptr<const ResultEntry>>& res);
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
