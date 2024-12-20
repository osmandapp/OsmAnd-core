#ifndef _OSMAND_CORE_REVERSE_GEOCODER_H_
#define _OSMAND_CORE_REVERSE_GEOCODER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QHash>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Data/Building.h>
#include <OsmAndCore/Data/StreetGroup.h>
#include <OsmAndCore/Data/Street.h>
//#include <OsmAndCore/IObfsCollection.h>
#include <OsmAndCore/IRoadLocator.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Nullable.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/Search/BaseSearch.h>

namespace OsmAnd
{
    class ReverseGeocoder_P;
    class OSMAND_CORE_API ReverseGeocoder Q_DECL_FINAL : public BaseSearch
    {
        Q_DISABLE_COPY_AND_MOVE(ReverseGeocoder)
    private:
        PrivateImplementation<ReverseGeocoder_P> _p;
    public:
        struct OSMAND_CORE_API Criteria : public BaseSearch::Criteria
        {
            Criteria();
            virtual ~Criteria();

            Nullable<LatLon> latLon;
            Nullable<PointI> position31;
        };

        struct OSMAND_CORE_API ResultEntry : public IResultEntry
        {
        public:
            ResultEntry();
            virtual ~ResultEntry();

            Nullable<LatLon> searchPoint;

            Nullable<LatLon> connectionPoint;
            QString streetName;

            std::shared_ptr<const Road> road;
            std::shared_ptr<const Building> building;
            QString buildingInterpolation;
            std::shared_ptr<const Street> street;
            std::shared_ptr<const StreetGroup> streetGroup;
            std::shared_ptr<const OsmAnd::RoadInfo> point;

            double getDistance() const;
            void resetDistance();
            double getCityDistance() const;
            Nullable<PointI> searchPoint31() const;
            void setDistance(double distance) const;
            QString toString() const;

        private:
            mutable double cityDist = -1;
            mutable double dist = -1;
        };

        explicit ReverseGeocoder(
                const std::shared_ptr<const IObfsCollection>& obfsCollection,
                const std::shared_ptr<const OsmAnd::IRoadLocator> &roadLocator);
        virtual ~ReverseGeocoder();

        virtual void performSearch(
                const ISearch::Criteria& criteria,
                const NewResultEntryCallback newResultEntryCallback,
                const std::shared_ptr<const IQueryController>& queryController = nullptr) const;
        std::shared_ptr<const ResultEntry> performSearch(const Criteria &criteria) const;
    };
}

#endif // _OSMAND_CORE_REVERSE_GEOCODER_H_
