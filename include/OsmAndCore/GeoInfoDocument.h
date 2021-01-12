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
    class OSMAND_CORE_API GeoInfoDocument
    {
        Q_DISABLE_COPY_AND_MOVE(GeoInfoDocument);
    public:
        struct OSMAND_CORE_API ExtraData
        {
            ExtraData();
            virtual ~ExtraData();

            virtual QHash<QString, QVariant> getValues(const bool recursive = true) const = 0;
        };

        struct OSMAND_CORE_API Link
        {
            Link();
            virtual ~Link();

            QUrl url;
            QString text;
        };
        
        struct OSMAND_CORE_API Author
        {
            Author();
            virtual ~Author();
            
            QString name;
            QString email;
            QString link;
        };
        
        struct OSMAND_CORE_API Copyright
        {
            Copyright();
            virtual ~Copyright();
            
            QString author;
            QString year;
            QString license;
        };
        
        struct OSMAND_CORE_API Bounds
        {
            Bounds();
            virtual ~Bounds();
            
            double minlat;
            double minlon;
            double maxlat;
            double maxlon;
        };

        struct OSMAND_CORE_API Metadata
        {
            Metadata();
            virtual ~Metadata();

            QString name;
            QString description;
            QList< Ref<Link> > links;
            QString keywords;
            QDateTime timestamp;
            Ref<ExtraData> extraData;
            Ref<Author> author;
            Ref<Copyright> copyright;
            Ref<Bounds> bounds;
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

        struct OSMAND_CORE_API TrackSegment
        {
            TrackSegment();
            virtual ~TrackSegment();

            QList< Ref<LocationMark> > points;
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

        struct OSMAND_CORE_API Route
        {
            Route();
            virtual ~Route();

            QString name;
            QString description;
            QString comment;
            QString type;
            QList< Ref<Link> > links;
            QList< Ref<LocationMark> > points;
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
        
        bool hasRtePt() const;
        bool hasWptPt() const;
        bool hasTrkPt() const;
    };
}

#endif // !defined(_OSMAND_CORE_GEO_INFO_DOCUMENT_H_)
