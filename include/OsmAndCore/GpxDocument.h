#ifndef _OSMAND_CORE_GPX_DOCUMENT_H_
#define _OSMAND_CORE_GPX_DOCUMENT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QHash>
#include <QIODevice>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/GeoInfoDocument.h>

namespace OsmAnd
{
    class OSMAND_CORE_API GpxDocument : public GeoInfoDocument
    {
        Q_DISABLE_COPY_AND_MOVE(GpxDocument);
    public:

        struct OSMAND_CORE_API GpxLink : public Link
        {
            GpxLink();
            virtual ~GpxLink();
        };

        struct OSMAND_CORE_API GpxMetadata : public Metadata
        {
            GpxMetadata();
            virtual ~GpxMetadata();
        };

        struct OSMAND_CORE_API GpxWptPt : public WptPt
        {
            GpxWptPt();
            virtual ~GpxWptPt();
        };

        struct OSMAND_CORE_API GpxTrkSeg : public TrackSegment
        {
            GpxTrkSeg();
            virtual ~GpxTrkSeg();
        };

        struct OSMAND_CORE_API GpxTrk : public Track
        {
            GpxTrk();
            virtual ~GpxTrk();
        };

        struct OSMAND_CORE_API GpxRte : public Route
        {
            GpxRte();
            virtual ~GpxRte();
        };

    private:
        static std::shared_ptr<GpxWptPt> parseWptAttributes(QXmlStreamReader& xmlReader);
        static std::shared_ptr<Bounds> parseBoundsAttributes(QXmlStreamReader& xmlReader);
        static QString getFilename(const QString& path);
        static void writeNotNullTextWithAttribute(QXmlStreamWriter& xmlWriter, const QString& tag, const QString &attribute, const QString& value);
        static void writeNotNullText(QXmlStreamWriter& xmlWriter, const QString& tag, const QString& value);
        static void writeAuthor(QXmlStreamWriter& xmlWriter, const Ref<Author>& author);
        static void writeCopyright(QXmlStreamWriter& xmlWriter, const Ref<Copyright>& copyright);
        static void writeBounds(QXmlStreamWriter& xmlWriter, const Ref<Bounds>& bounds);
        static std::shared_ptr<RouteSegment> parseRouteSegmentAttributes(QXmlStreamReader& parser);
        static std::shared_ptr<RouteType> parseRouteTypeAttributes(QXmlStreamReader& parser);
        static QMap<QString, QString> readTextMap(QXmlStreamReader &xmlReader, QString key);
        static QString readText(QXmlStreamReader& xmlReader, QString key);
    protected:
        static void writeLinks(const QList< Ref<Link> >& links, QXmlStreamWriter& xmlWriter);
        static void writeExtensions(const QList< Ref<Extension> > &extensions, const QHash<QString, QString> &attributes, QXmlStreamWriter& xmlWriter);
        static void writeExtension(const std::shared_ptr<const Extension>& extension, QXmlStreamWriter& xmlWriter);
    public:
        GpxDocument();
        virtual ~GpxDocument();

        QString version;
        QString creator;

        static std::shared_ptr<GpxDocument> createFrom(const std::shared_ptr<const GeoInfoDocument>& document);

        bool saveTo(QXmlStreamWriter& xmlWriter, const QString& filename) const;
        bool saveTo(QIODevice& ioDevice, const QString& filename) const;
        bool saveTo(const QString& filename) const;
        static std::shared_ptr<GpxDocument> loadFrom(QXmlStreamReader& parser);
        static std::shared_ptr<GpxDocument> loadFrom(QIODevice& ioDevice);
        static std::shared_ptr<GpxDocument> loadFrom(const QString& filename);
    };
}

#endif // !defined(_OSMAND_CORE_GPX_DOCUMENT_H_)
