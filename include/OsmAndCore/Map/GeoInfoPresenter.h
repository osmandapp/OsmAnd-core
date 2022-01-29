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
            class OSMAND_CORE_API AttributeMapping : public OsmAnd::MapObject::AttributeMapping
            {
                Q_DISABLE_COPY_AND_MOVE(AttributeMapping);

            private:
            protected:
                virtual void registerRequiredMapping(uint32_t& lastUsedEntryId);
            public:
                AttributeMapping();
                virtual ~AttributeMapping();

                uint32_t waypointAttributeId;
                uint32_t trackpointAttributeId;
                uint32_t routepointAttributeId;
                uint32_t tracklineAttributeId;
                uint32_t routelineAttributeId;
                uint32_t waypointsPresentAttributeId;
                uint32_t waypointsNotPresentAttributeId;
                uint32_t trackpointsPresentAttributeId;
                uint32_t trackpointsNotPresentAttributeId;
                uint32_t routepointsPresentAttributeId;
                uint32_t routepointsNotPresentAttributeId;

                virtual void registerMapping(const uint32_t id, const QString& tag, const QString& value);
            };

        private:
        protected:
            MapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const Extensions>& extensions = nullptr);
        public:
            virtual ~MapObject();

            const std::shared_ptr<const GeoInfoDocument> geoInfoDocument;
        };

        class OSMAND_CORE_API WaypointMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(WaypointMapObject);

        public:
            WaypointMapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const GeoInfoDocument::WptPt>& waypoint);
            virtual ~WaypointMapObject();

            const std::shared_ptr<const GeoInfoDocument::WptPt> waypoint;
        };

        class OSMAND_CORE_API TrackpointMapObject : public MapObject
        {
            Q_DISABLE_COPY_AND_MOVE(TrackpointMapObject);

        public:
            TrackpointMapObject(
                const std::shared_ptr<const GeoInfoDocument>& geoInfoDocument,
                const std::shared_ptr<const GeoInfoDocument::Track>& track,
                const std::shared_ptr<const GeoInfoDocument::TrackSegment>& trackSegment,
                const std::shared_ptr<const GeoInfoDocument::WptPt>& trackpoint);
            virtual ~TrackpointMapObject();

            const std::shared_ptr<const GeoInfoDocument::Track> track;
            const std::shared_ptr<const GeoInfoDocument::TrackSegment> trackSegment;
            const std::shared_ptr<const GeoInfoDocument::WptPt> trackpoint;
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
                const std::shared_ptr<const GeoInfoDocument::WptPt>& routepoint);
            virtual ~RoutepointMapObject();

            const std::shared_ptr<const GeoInfoDocument::Route> route;
            const std::shared_ptr<const GeoInfoDocument::WptPt> routepoint;
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
