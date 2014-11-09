#ifndef _OSMAND_CORE_GEO_INFO_DOCUMENT_H_
#define _OSMAND_CORE_GEO_INFO_DOCUMENT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QUrl>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Ref.h>

namespace OsmAnd
{
    class OSMAND_CORE_API GeoInfoDocument
    {
        Q_DISABLE_COPY_AND_MOVE(GeoInfoDocument);
    public:
        struct OSMAND_CORE_API ExtraData
        {
            ExtraData();
            virtual ~ExtraData();
        };

        struct OSMAND_CORE_API Link
        {
            Link();
            virtual ~Link();

            QUrl url;
            QString text;
        };

        struct OSMAND_CORE_API Metadata
        {
            Metadata();
            virtual ~Metadata();

            QString name;
            QString description;
            QList< Ref<Link> > links;
            QDateTime timestamp;
            Ref<ExtraData> extraData;
        };

        struct OSMAND_CORE_API LocationMark
        {
            LocationMark();
            virtual ~LocationMark();

            LatLon position;
            QString name;
            QString description;
            double elevation;
            QDateTime timestamp;
            QString comment;
            QString type;
            QList< Ref<Link> > links;
            Ref<ExtraData> extraData;
        };

        struct OSMAND_CORE_API TrackPoint : public LocationMark
        {
            TrackPoint();
            virtual ~TrackPoint();
        };

        struct OSMAND_CORE_API TrackSegment
        {
            TrackSegment();
            virtual ~TrackSegment();

            QList< Ref<TrackPoint> > points;
            Ref<ExtraData> extraData;
        };

        struct OSMAND_CORE_API Track
        {
            Track();
            virtual ~Track();

            QString name;
            QString description;
            QString comment;
            QString type;
            QList< Ref<Link> > links;
            QList< Ref<TrackSegment> > segments;
            Ref<ExtraData> extraData;
        };

        struct OSMAND_CORE_API RoutePoint : public LocationMark
        {
            RoutePoint();
            virtual ~RoutePoint();
        };

        struct OSMAND_CORE_API Route
        {
            Route();
            virtual ~Route();

            QString name;
            QString description;
            QString comment;
            QString type;
            QList< Ref<Link> > links;
            QList< Ref<RoutePoint> > points;
            Ref<ExtraData> extraData;
        };

    private:
    protected:
    public:
        GeoInfoDocument();
        virtual ~GeoInfoDocument();

        Ref<Metadata> metadata;
        QList< Ref<LocationMark> > locationMarks;
        QList< Ref<Track> > tracks;
        QList< Ref<Route> > routes;
        Ref<ExtraData> extraData;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_INFO_DOCUMENT_H_)
