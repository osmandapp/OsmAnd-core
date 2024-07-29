#ifndef _OSMAND_CORE_GPX_DOCUMENT_H_
#define _OSMAND_CORE_GPX_DOCUMENT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <QDateTime>
#include <QUrl>
#include <QHash>
#include <QVariant>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/PointsAndAreas.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Ref.h>
#include <OsmAndCore/Color.h>

namespace OsmAnd
{
    class OSMAND_CORE_API GpxExtensions {
        Q_DISABLE_COPY_AND_MOVE(GpxExtensions);
    public:

        struct OSMAND_CORE_API GpxExtension {
            GpxExtension();
            virtual ~GpxExtension();

            QString prefix;
            QString name;
            QString value;
            QHash<QString, QString> attributes;
            QList< Ref<GpxExtension> > subextensions;

            QHash<QString, QVariant> getValues(const bool recursive = true) const;
        };

    private:
    protected:
    public:
        GpxExtensions();
        virtual ~GpxExtensions();

        QString value;
        QHash<QString, QString> attributes;
        QList< Ref<GpxExtension> > extensions;

        QHash<QString, QVariant> getValues(const bool recursive = true) const;
    };

    class OSMAND_CORE_API GpxDocument : public GpxExtensions
    {
        Q_DISABLE_COPY_AND_MOVE(GpxDocument);
    public:

        struct OSMAND_CORE_API Link : public GpxExtensions
        {
            Link();
            virtual ~Link();

            QUrl url;
            QString text;
        };

        struct OSMAND_CORE_API Author : public GpxExtensions
        {
            Author();
            virtual ~Author();

            QString name;
            QString email;
            Ref<Link> link;
        };

        struct OSMAND_CORE_API Copyright : public GpxExtensions
        {
            Copyright();
            virtual ~Copyright();

            QString author;
            QString year;
            QString license;
        };

        struct OSMAND_CORE_API Bounds : public GpxExtensions
        {
            Bounds();
            virtual ~Bounds();

            double minlat;
            double minlon;
            double maxlat;
            double maxlon;
        };

        struct OSMAND_CORE_API Metadata : public GpxExtensions
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

        struct OSMAND_CORE_API WptPt : public GpxExtensions
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
            double heading;
        };

        struct OSMAND_CORE_API RouteSegment
        {
            RouteSegment();
            virtual ~RouteSegment();

            QString id;
            QString length;
            QString startTrackPointIndex;
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

        struct OSMAND_CORE_API PointsGroup
        {
            PointsGroup();
            virtual ~PointsGroup();

            QString name;
            QString iconName;
            QString backgroundType;
            QList< Ref<WptPt> > points;
            ColorARGB color;
        };

        struct OSMAND_CORE_API TrkSegment : public GpxExtensions
        {
            TrkSegment();
            virtual ~TrkSegment();

            QString name;
            QList< Ref<WptPt> > points;
            QList< Ref<RouteSegment> > routeSegments;
            QList< Ref<RouteType> > routeTypes;
        };

        struct OSMAND_CORE_API Track : public GpxExtensions
        {
            Track();
            virtual ~Track();

            QString name;
            QString description;
            QList< Ref<TrkSegment> > segments;
        };

        struct OSMAND_CORE_API Route : public GpxExtensions
        {
            Route();
            virtual ~Route();

            QString name;
            QString description;
            QList< Ref<WptPt> > points;
        };

    private:
        static std::shared_ptr<WptPt> parseWptAttributes(QXmlStreamReader& xmlReader);
        static std::shared_ptr<Bounds> parseBoundsAttributes(QXmlStreamReader& xmlReader);
        static QString getFilename(const QString& path);
        static void writeNotNullTextWithAttribute(QXmlStreamWriter& xmlWriter, const QString& tag, const QString &attribute, const QString& value);
        static void writeNotNullText(QXmlStreamWriter& xmlWriter, const QString& tag, const QString& value);
        static void writeAuthor(QXmlStreamWriter& xmlWriter, const Ref<Author>& author);
        static void writeCopyright(QXmlStreamWriter& xmlWriter, const Ref<Copyright>& copyright);
        static void writeBounds(QXmlStreamWriter& xmlWriter, const Ref<Bounds>& bounds);
        static std::shared_ptr<RouteSegment> parseRouteSegmentAttributes(QXmlStreamReader& parser);
        static std::shared_ptr<RouteType> parseRouteTypeAttributes(QXmlStreamReader& parser);
        static std::shared_ptr<PointsGroup> parsePointsGroupAttributes(QXmlStreamReader& parser);
        static QMap<QString, QString> readTextMap(QXmlStreamReader &xmlReader, QString key);
        static QMap<QString, Ref<PointsGroup> > mergePointsGroups(QList< Ref<PointsGroup> > &groups, QList< Ref<WptPt> > &points);
        static QString readText(QXmlStreamReader& xmlReader, QString key);
        static bool containsExtension(const QList<Ref<GpxExtension>> &extensions, const QString& name, const QString& value);
    protected:
        static void writeLinks(const QList< Ref<Link> >& links, QXmlStreamWriter& xmlWriter);
        static void writeExtensions(const QList< Ref<GpxExtension> > &extensions, const QHash<QString, QString> &attributes, QXmlStreamWriter& xmlWriter);
        static void writeExtension(const std::shared_ptr<const GpxExtension>& extension, QXmlStreamWriter& xmlWriter, const QString &namesp);
        static void writeMetadata(const Ref<Metadata>& metadata, const QString& filename, QXmlStreamWriter& xmlWriter);
        static void writePoints(const QList< Ref<WptPt> > &points, QXmlStreamWriter& xmlWriter);
        static void writeRoutes(const QList< Ref<Route> > &routes, QXmlStreamWriter& xmlWriter);
        static void writeTracks(const QList< Ref<Track> > &tracks, QXmlStreamWriter& xmlWriter);
        static void writeWpt(const Ref<WptPt> &p, const QString &elementName, QXmlStreamWriter& xmlWriter);
    public:
        GpxDocument();
        virtual ~GpxDocument();

        QString version;
        QString creator;
        Ref<Metadata> metadata;
        QList< Ref<Track> > tracks;
        QList< Ref<WptPt> > points;
        QList< Ref<Route> > routes;
        QMap<QString, Ref<PointsGroup> > pointsGroups;
        QMap<QString, QString> networkRouteKeyTags;

        bool hasRtePt() const;
        bool hasWptPt() const;
        bool hasTrkPt() const;

        static std::shared_ptr<GpxDocument> createFrom(const std::shared_ptr<const GpxDocument>& document);

        bool saveTo(QXmlStreamWriter& xmlWriter, const QString& filename, const QString& creatorName = QStringLiteral("OsmAnd Core")) const;
        bool saveTo(QIODevice& ioDevice, const QString& filename, const QString& creatorName = QStringLiteral("OsmAnd Core")) const;
        bool saveTo(const QString& filename, const QString& creatorName = QStringLiteral("OsmAnd Core")) const;
        static std::shared_ptr<GpxDocument> loadFrom(QXmlStreamReader& parser);
        static std::shared_ptr<GpxDocument> loadFrom(QIODevice& ioDevice);
        static std::shared_ptr<GpxDocument> loadFrom(const QString& filename);
    };
}

#endif // !defined(_OSMAND_CORE_GPX_DOCUMENT_H_)
