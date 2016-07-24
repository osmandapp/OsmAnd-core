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
        Q_DISABLE_COPY_AND_MOVE(GpxDocument)
    public:
        struct OSMAND_CORE_API GpxExtension : public ExtraData
        {
            GpxExtension();
            virtual ~GpxExtension();

            QString name;
            QString value;
            QHash<QString, QString> attributes;
            QList< Ref<GpxExtension> > subextensions;

            virtual QHash<QString, QVariant> getValues(const bool recursive = true) const Q_DECL_OVERRIDE;
        };

        struct OSMAND_CORE_API GpxExtensions : public ExtraData
        {
            GpxExtensions();
            virtual ~GpxExtensions();

            QHash<QString, QString> attributes;
            QString value;
            QList< Ref<GpxExtension> > extensions;

            virtual QHash<QString, QVariant> getValues(const bool recursive = true) const Q_DECL_OVERRIDE;
        };

        struct OSMAND_CORE_API GpxLink : public Link
        {
            GpxLink();
            virtual ~GpxLink();

            QString type;
        };

        struct OSMAND_CORE_API GpxMetadata : public Metadata
        {
            GpxMetadata();
            virtual ~GpxMetadata();
        };

        enum class GpxFixType
        {
            // Fix type unknown
            Unknown = -1,

            // No fix
            None,

            // Position only, or 2D
            PositionOnly,

            // Position and Elevation, or 3D
            PositionAndElevation,

            // DGPS
            DGPS,

            // Military signal used
            PPS
        };

        struct OSMAND_CORE_API GpxWpt : public LocationMark
        {
            GpxWpt();
            virtual ~GpxWpt();

            double magneticVariation;
            double geoidHeight;
            QString source;
            QString category;
            QString symbol;
            GpxFixType fixType;
            int satellitesUsedForFixCalculation;
            double horizontalDilutionOfPrecision;
            double verticalDilutionOfPrecision;
            double positionDilutionOfPrecision;
            double ageOfGpsData;
            int dgpsStationId;
        };

        struct OSMAND_CORE_API GpxTrkPt : public GpxWpt
        {
            GpxTrkPt();
            virtual ~GpxTrkPt();

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

            QString source;
            int slotNumber;
        };

        struct OSMAND_CORE_API GpxRtePt : public GpxWpt
        {
            GpxRtePt();
            virtual ~GpxRtePt();
        };

        struct OSMAND_CORE_API GpxRte : public Route
        {
            GpxRte();
            virtual ~GpxRte();

            QString source;
            int slotNumber;
        };

    private:
    protected:
        static void writeLinks(const QList< Ref<Link> >& links, QXmlStreamWriter& xmlWriter);
        static void writeExtensions(const std::shared_ptr<const GpxExtensions>& extensions, QXmlStreamWriter& xmlWriter);
        static void writeExtension(const std::shared_ptr<const GpxExtension>& extension, QXmlStreamWriter& xmlWriter);
    public:
        GpxDocument();
        virtual ~GpxDocument();

        QString version;
        QString creator;

        static std::shared_ptr<GpxDocument> createFrom(const std::shared_ptr<const GeoInfoDocument>& document);

        bool saveTo(QXmlStreamWriter& xmlWriter) const;
        bool saveTo(QIODevice& ioDevice) const;
        bool saveTo(const QString& filename) const;
        static std::shared_ptr<GpxDocument> loadFrom(QXmlStreamReader& xmlReader);
        static std::shared_ptr<GpxDocument> loadFrom(QIODevice& ioDevice);
        static std::shared_ptr<GpxDocument> loadFrom(const QString& filename);
    };
}

#endif // !defined(_OSMAND_CORE_GPX_DOCUMENT_H_)
