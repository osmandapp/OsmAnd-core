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
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/Data/Street.h>
#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/IRoadLocator.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Search/AddressesByNameSearch.h>
#include <OsmAndCore/Search/BaseSearch.h>

namespace OsmAnd
{
    class OSMAND_CORE_API ReverseGeocoder Q_DECL_FINAL : public BaseSearch
    {
        Q_DISABLE_COPY_AND_MOVE(ReverseGeocoder)
    public:
        struct OSMAND_CORE_API Criteria : public BaseSearch::Criteria
        {
            Nullable<LatLon> latLon;
            Nullable<PointI> position31;
        };

        struct OSMAND_CORE_API ResultEntry : public IResultEntry
        {
        public:
            Nullable<LatLon> searchPoint;

            Nullable<LatLon> connectionPoint;
//            int regionFP;
//            int regionLen;
//            Nullable<const LatLon>& point;  // RouteSegmentPoint in Java
            QString streetName;

            std::shared_ptr<const Building> building;
            QString buildingInterpolation;
            std::shared_ptr<const Street> street;
            std::shared_ptr<const StreetGroup> streetGroup;

            double getDistance() const;
            AreaI searchBbox() const;
            Nullable<PointI> searchPoint31() const;
            void setDistance(double dist);
            QString toString() const;

//            inline double getDistanceP() {
//                if (point != null && searchPoint != null) {
//                    // Need distance between searchPoint and nearest RouteSegmentPoint here, to approximate distance from nearest named road
//                    return Math.sqrt(point.distSquare);
//                } else {
//                    return NAN;
//                }
//            }

//        private:
            double dist = NAN;
        };

        explicit ReverseGeocoder(
                const std::shared_ptr<const IObfsCollection>& obfsCollection,
                const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator);

        virtual void performSearch(
            const ISearch::Criteria& criteria,
            const NewResultEntryCallback newResultEntryCallback,
                const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
    private:
        const std::shared_ptr<const IRoadLocator> roadLocator;

        static bool DISTANCE_COMPARATOR(
                const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> &a,
                const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> &b);

        AddressesByNameSearch* addressByNameSearch;
        std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> justifyResult(QVector<std::shared_ptr<const ResultEntry> > res) const;
        QVector<std::shared_ptr<const ResultEntry> > justifyReverseGeocodingSearch(
                const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> &road,
                double knownMinBuildingDistance) const;
        QVector<std::shared_ptr<const ResultEntry>> loadStreetBuildings(
                const std::shared_ptr<const OsmAnd::ReverseGeocoder::ResultEntry> road,
                const std::shared_ptr<const ResultEntry> street) const;
        QVector<std::shared_ptr<const ResultEntry>> reverseGeocodeToRoads(
                const LatLon searchPoint) const;
    };
}

#endif // REVERSEGEOCODER_H
