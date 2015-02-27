#ifndef _OSMAND_CORE_GEO_INFO_PRESENTER_H_
#define _OSMAND_CORE_GEO_INFO_PRESENTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/GeoInfoDocument.h>
#include <OsmAndCore/Data/MapObject.h>

namespace OsmAnd
{
    class IMapObjectsProvider;

    class GeoInfoPresenter_P;
    class OSMAND_CORE_API GeoInfoPresenter
    {
        Q_DISABLE_COPY_AND_MOVE(GeoInfoPresenter);

    public:
        class OSMAND_CORE_API MapObject : public OsmAnd::MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(MapObject);

        public:
            class OSMAND_CORE_API EncodingDecodingRules : public OsmAnd::MapObject::EncodingDecodingRules
            {
                Q_DISABLE_COPY_AND_MOVE(EncodingDecodingRules);

            private:
            protected:
                virtual void createRequiredRules(uint32_t& lastUsedRuleId);
            public:
                EncodingDecodingRules();
                virtual ~EncodingDecodingRules();

                // Quick-access rules
                uint32_t waypoint_encodingRuleId;
                uint32_t trackpoint_encodingRuleId;
                uint32_t routepoint_encodingRuleId;
                uint32_t trackline_encodingRuleId;
                uint32_t routeline_encodingRuleId;
                uint32_t waypointsPresent_encodingRuleId;
                uint32_t waypointsNotPresent_encodingRuleId;
                uint32_t trackpointsPresent_encodingRuleId;
                uint32_t trackpointsNotPresent_encodingRuleId;
                uint32_t routepointsPresent_encodingRuleId;
                uint32_t routepointsNotPresent_encodingRuleId;

                virtual uint32_t addRule(const uint32_t ruleId, const QString& ruleTag, const QString& ruleValue);
            };

        private:
        protected:
            MapObject(const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument);
        public:
            virtual ~MapObject();

            const std::shared_ptr<const GeoInfoDocument> geoInfoDocument;

            // Default encoding-decoding rules
            static std::shared_ptr<const EncodingDecodingRules> defaultEncodingDecodingRules;
        };

        class OSMAND_CORE_API WaypointMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(WaypointMapObject);

        public:
            WaypointMapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const GeoInfoDocument::LocationMark>& waypoint);
            virtual ~WaypointMapObject();

            const std::shared_ptr<const GeoInfoDocument::LocationMark> waypoint;
        };

        class OSMAND_CORE_API TrackpointMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(TrackpointMapObject);

        public:
            TrackpointMapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const GeoInfoDocument::Track>& track,
                const std::shared_ptr<const GeoInfoDocument::TrackSegment>& trackSegment,
                const std::shared_ptr<const GeoInfoDocument::LocationMark>& trackpoint);
            virtual ~TrackpointMapObject();

            const std::shared_ptr<const GeoInfoDocument::Track> track;
            const std::shared_ptr<const GeoInfoDocument::TrackSegment> trackSegment;
            const std::shared_ptr<const GeoInfoDocument::LocationMark> trackpoint;
        };

        class OSMAND_CORE_API TracklineMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(TracklineMapObject);

        public:
            TracklineMapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const GeoInfoDocument::Track>& track,
                const std::shared_ptr<const GeoInfoDocument::TrackSegment>& trackSegment);
            virtual ~TracklineMapObject();

            const std::shared_ptr<const GeoInfoDocument::Track> track;
            const std::shared_ptr<const GeoInfoDocument::TrackSegment> trackSegment;
        };

        class OSMAND_CORE_API RoutepointMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(RoutepointMapObject);

        public:
            RoutepointMapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const GeoInfoDocument::Route>& route,
                const std::shared_ptr<const GeoInfoDocument::LocationMark>& routepoint);
            virtual ~RoutepointMapObject();

            const std::shared_ptr<const GeoInfoDocument::Route> route;
            const std::shared_ptr<const GeoInfoDocument::LocationMark> routepoint;
        };

        class OSMAND_CORE_API RoutelineMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(RoutelineMapObject);

        public:
            RoutelineMapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const GeoInfoDocument::Route>& route);
            virtual ~RoutelineMapObject();

            const std::shared_ptr<const GeoInfoDocument::Route> route;
        };

    private:
        PrivateImplementation<GeoInfoPresenter_P> _p;
    protected:
    public:
        GeoInfoPresenter(
            const QList< std::shared_ptr<const GeoInfoDocument> >& documents);
        virtual ~GeoInfoPresenter();

        const QList< std::shared_ptr<const GeoInfoDocument> > documents;

        std::shared_ptr<IMapObjectsProvider> createMapObjectsProvider() const;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_INFO_PRESENTER_H_)
