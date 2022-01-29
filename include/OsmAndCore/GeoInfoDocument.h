#ifndef _OSMAND_CORE_GEO_INFO_DOCUMENT_H_
#define _OSMAND_CORE_GEO_INFO_DOCUMENT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QUrl>
#include <QHash>
#include <QVariant>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Ref.h>

namespace OsmAnd
{
    class OSMAND_CORE_API Extensions {
        Q_DISABLE_COPY_AND_MOVE(Extensions);
    public:

        struct OSMAND_CORE_API Extension {
            Extension();

            virtual ~Extension();

            QString name;
            QString value;
            QHash<QString, QString> attributes;
            QList< Ref<Extension> > subextensions;

            QHash<QString, QVariant> getValues(const bool recursive = true) const;
        };

    private:
    protected:
    public:
        Extensions();
        virtual ~Extensions();

        QString value;
        QHash<QString, QString> attributes;
        QList< Ref<Extension> > extensions;

        QHash<QString, QVariant> getValues(const bool recursive = true) const;
    };
}

namespace OsmAnd
{
    class OSMAND_CORE_API GeoInfoDocument : public Extensions
    {
        Q_DISABLE_COPY_AND_MOVE(GeoInfoDocument);
    public:

        struct OSMAND_CORE_API Link
        {
            Link();
            virtual ~Link();

            QUrl url;
            QString text;
        };
        
        struct OSMAND_CORE_API Author : public Extensions
        {
            Author();
            virtual ~Author();
            
            QString name;
            QString email;
            QString link;
        };
        
        struct OSMAND_CORE_API Copyright : public Extensions
        {
            Copyright();
            virtual ~Copyright();
            
            QString author;
            QString year;
            QString license;
        };
        
        struct OSMAND_CORE_API Bounds : public Extensions
        {
            Bounds();
            virtual ~Bounds();
            
            double minlat;
            double minlon;
            double maxlat;
            double maxlon;
        };

        struct OSMAND_CORE_API Metadata : public Extensions
        {
            Metadata();
            virtual ~Metadata();

            QString name;
            QString description;
            QList< Ref<Link> > links;
            QString keywords;
            QDateTime timestamp;
            Ref<Author> author;
            Ref<Copyright> copyright;
            Ref<Bounds> bounds;
        };

        struct OSMAND_CORE_API WptPt : public Extensions
        {
            WptPt();
            virtual ~WptPt();

            LatLon position;
            QString name;
            QString description;
            double elevation;
            QDateTime timestamp;
            QString comment;
            QString type;
            QList< Ref<Link> > links;
            double horizontalDilutionOfPrecision;
            double verticalDilutionOfPrecision;
            double speed;
        };

        struct OSMAND_CORE_API RouteSegment
        {
            RouteSegment();
            virtual ~RouteSegment();

            QString id;
            QString length;
            QString segmentTime;
            QString speed;
            QString turnType;
            QString turnAngle;
            QString types;
            QString pointTypes;
            QString names;
        };

        struct OSMAND_CORE_API RouteType
        {
            RouteType();
            virtual ~RouteType();

            QString tag;
            QString value;
        };

        struct OSMAND_CORE_API TrackSegment : public Extensions
        {
            TrackSegment();
            virtual ~TrackSegment();

            QString name;
            QList< Ref<WptPt> > points;
            QList< Ref<RouteSegment> > routeSegments;
            QList< Ref<RouteType> > routeTypes;
        };

        struct OSMAND_CORE_API Track : public Extensions
        {
            Track();
            virtual ~Track();

            QString name;
            QString description;
            QList< Ref<TrackSegment> > segments;
        };

        struct OSMAND_CORE_API Route : public Extensions
        {
            Route();
            virtual ~Route();

            QString name;
            QString description;
            QList< Ref<WptPt> > points;
        };

    private:
    protected:
    public:
        GeoInfoDocument();
        virtual ~GeoInfoDocument();

        Ref<Metadata> metadata;
        QList< Ref<WptPt> > points;
        QList< Ref<Track> > tracks;
        QList< Ref<Route> > routes;

        bool hasRtePt() const;
        bool hasWptPt() const;
        bool hasTrkPt() const;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_INFO_DOCUMENT_H_)
