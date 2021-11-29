#include "GpxDocument.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QFile>
#include <QStack>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "Common.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::GpxDocument::GpxDocument()
{
}

OsmAnd::GpxDocument::~GpxDocument()
{
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::createFrom(const std::shared_ptr<const GeoInfoDocument>& document)
{
    return nullptr;
}

bool OsmAnd::GpxDocument::saveTo(QXmlStreamWriter& xmlWriter, const QString& filename) const
{
    xmlWriter.writeStartDocument(QStringLiteral("1.0"), true);

    //<gpx
    //	  version="1.1"
    //	  creator="OsmAnd"
    //	  xmlns="http://www.topografix.com/GPX/1/1"
    //	  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    //	  xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
    xmlWriter.writeStartElement(QStringLiteral("gpx"));
    xmlWriter.writeAttribute(QStringLiteral("version"), version.isEmpty() ? QStringLiteral("1.1") : version);
    xmlWriter.writeAttribute(QStringLiteral("creator"), creator.isEmpty() ? QStringLiteral("OsmAnd Core") : creator);
    xmlWriter.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.topografix.com/GPX/1/1"));
    xmlWriter.writeAttribute(QStringLiteral("xmlns:xsi"), QStringLiteral("http://www.w3.org/2001/XMLSchema-instance"));
    xmlWriter.writeAttribute(QStringLiteral("xsi:schemaLocation"), QStringLiteral("http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd"));

    QString trackName = metadata != nullptr && !metadata->name.isEmpty() ? metadata->name : getFilename(filename);
    // <metadata>
    xmlWriter.writeStartElement(QStringLiteral("metadata"));
    writeNotNullText(xmlWriter, QStringLiteral("name"), trackName);
    // Write metadata
    if (metadata)
    {
        // <desc>
        writeNotNullText(xmlWriter, QStringLiteral("desc"), metadata->description);
        
        if (metadata->author)
        {
            xmlWriter.writeStartElement(QStringLiteral("author"));
            writeAuthor(xmlWriter, metadata->author);
            xmlWriter.writeEndElement();
        }
        
        if (metadata->copyright)
        {
            xmlWriter.writeStartElement(QStringLiteral("copyright"));
            writeCopyright(xmlWriter, metadata->copyright);
            xmlWriter.writeEndElement();
        }
        
        writeNotNullText(xmlWriter, "keywords", metadata->keywords);
        if (metadata->bounds)
            writeBounds(xmlWriter, metadata->bounds);

        // Links
        if (!metadata->links.isEmpty())
            writeLinks(metadata->links, xmlWriter);

        // <time>
        if (!metadata->timestamp.isNull())
            xmlWriter.writeTextElement(QStringLiteral("time"), metadata->timestamp.toString(Qt::DateFormat::ISODate));

        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(metadata->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);
    }
    // </metadata>
    xmlWriter.writeEndElement();

    // <wpt>'s
    for (const auto& locationMark : constOf(locationMarks))
    {
        // <wpt>
        xmlWriter.writeStartElement(QStringLiteral("wpt"));
        xmlWriter.writeAttribute(QStringLiteral("lat"), QString::number(locationMark->position.latitude, 'f', 7));
        xmlWriter.writeAttribute(QStringLiteral("lon"), QString::number(locationMark->position.longitude, 'f', 7));

        // <name>
        if (!locationMark->name.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("name"), locationMark->name);

        // <desc>
        if (!locationMark->description.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("desc"), locationMark->description);

        // <ele>
        if (!qIsNaN(locationMark->elevation))
            xmlWriter.writeTextElement(QStringLiteral("ele"), QString::number(locationMark->elevation, 'g', 7));

        // <time>
        if (!locationMark->timestamp.isNull())
            xmlWriter.writeTextElement(QStringLiteral("time"), locationMark->timestamp.toString(Qt::DateFormat::ISODate));

        // <cmt>
        if (!locationMark->comment.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("cmt"), locationMark->comment);

        // <type>
        if (!locationMark->type.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("type"), locationMark->type);

        // Links
        if (!locationMark->links.isEmpty())
            writeLinks(locationMark->links, xmlWriter);

        if (const auto wpt = std::dynamic_pointer_cast<const GpxWpt>(locationMark.shared_ptr()))
        {
            // <magvar>
            if (!qIsNaN(wpt->magneticVariation))
                xmlWriter.writeTextElement(QStringLiteral("magvar"), QString::number(wpt->magneticVariation, 'f', 12));

            // <geoidheight>
            if (!qIsNaN(wpt->geoidHeight))
                xmlWriter.writeTextElement(QStringLiteral("geoidheight"), QString::number(wpt->geoidHeight, 'f', 12));

            // <src>
            if (!wpt->source.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("src"), wpt->source);

            // <sym>
            if (!wpt->symbol.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("sym"), wpt->symbol);

            // <fix>
            switch (wpt->fixType)
            {
                case GpxFixType::None:
                    xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("none"));
                    break;

                case GpxFixType::PositionOnly:
                    xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("2d"));
                    break;

                case GpxFixType::PositionAndElevation:
                    xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("3d"));
                    break;

                case GpxFixType::DGPS:
                    xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("dgps"));
                    break;

                case GpxFixType::PPS:
                    xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("pps"));
                    break;

                case GpxFixType::Unknown:
                default:
                    break;
            }

            // <sat>
            if (wpt->satellitesUsedForFixCalculation >= 0)
                xmlWriter.writeTextElement(QStringLiteral("sat"), QString::number(wpt->satellitesUsedForFixCalculation));

            // <hdop>
            if (!qIsNaN(wpt->horizontalDilutionOfPrecision))
                xmlWriter.writeTextElement(QStringLiteral("hdop"), QString::number(wpt->horizontalDilutionOfPrecision, 'f', 12));

            // <vdop>
            if (!qIsNaN(wpt->verticalDilutionOfPrecision))
                xmlWriter.writeTextElement(QStringLiteral("vdop"), QString::number(wpt->verticalDilutionOfPrecision, 'f', 12));

            // <pdop>
            if (!qIsNaN(wpt->positionDilutionOfPrecision))
                xmlWriter.writeTextElement(QStringLiteral("pdop"), QString::number(wpt->positionDilutionOfPrecision, 'f', 12));

            // <ageofdgpsdata>
            if (!qIsNaN(wpt->ageOfGpsData))
                xmlWriter.writeTextElement(QStringLiteral("ageofdgpsdata"), QString::number(wpt->ageOfGpsData, 'f', 12));

            // <dgpsid>
            if (wpt->dgpsStationId >= 0)
                xmlWriter.writeTextElement(QStringLiteral("dgpsid"), QString::number(wpt->dgpsStationId));
        }

        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(locationMark->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);

        // </wpt>
        xmlWriter.writeEndElement();
    }

    // <trk>'s
    for (const auto& track : constOf(tracks))
    {
        // <trk>
        xmlWriter.writeStartElement(QStringLiteral("trk"));

        // <name>
        if (!track->name.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("name"), track->name);

        // <desc>
        if (!track->description.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("desc"), track->description);

        // <cmt>
        if (!track->comment.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("cmt"), track->comment);

        // <type>
        if (!track->type.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("type"), track->type);

        // Links
        if (!track->links.isEmpty())
            writeLinks(track->links, xmlWriter);

        if (const auto trk = std::dynamic_pointer_cast<const GpxTrk>(track.shared_ptr()))
        {
            // <src>
            if (!trk->source.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("src"), trk->source);
        }

        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(track->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);

        // Write track segments
        for (const auto& trackSegment : constOf(track->segments))
        {
            // <trkseg>
            xmlWriter.writeStartElement(QStringLiteral("trkseg"));

            // Write track points
            for (const auto& trackPoint : constOf(trackSegment->points))
            {
                // <trkpt>
                xmlWriter.writeStartElement(QStringLiteral("trkpt"));
                xmlWriter.writeAttribute(QStringLiteral("lat"), QString::number(trackPoint->position.latitude, 'f', 7));
                xmlWriter.writeAttribute(QStringLiteral("lon"), QString::number(trackPoint->position.longitude, 'f', 7));

                // <name>
                if (!trackPoint->name.isEmpty())
                    xmlWriter.writeTextElement(QStringLiteral("name"), trackPoint->name);

                // <desc>
                if (!trackPoint->description.isEmpty())
                    xmlWriter.writeTextElement(QStringLiteral("desc"), trackPoint->description);

                // <ele>
                if (!qIsNaN(trackPoint->elevation))
                    xmlWriter.writeTextElement(QStringLiteral("ele"), QString::number(trackPoint->elevation, 'g', 7));

                // <time>
                if (!trackPoint->timestamp.isNull())
                    xmlWriter.writeTextElement(QStringLiteral("time"), trackPoint->timestamp.toString(Qt::DateFormat::ISODate));

                // <cmt>
                if (!trackPoint->comment.isEmpty())
                    xmlWriter.writeTextElement(QStringLiteral("cmt"), trackPoint->comment);

                // <type>
                if (!trackPoint->type.isEmpty())
                    xmlWriter.writeTextElement(QStringLiteral("type"), trackPoint->type);

                // Links
                if (!trackPoint->links.isEmpty())
                    writeLinks(trackPoint->links, xmlWriter);

                if (const auto trkpt = std::dynamic_pointer_cast<const GpxTrkPt>(trackPoint.shared_ptr()))
                {
                    // <magvar>
                    if (!qIsNaN(trkpt->magneticVariation))
                        xmlWriter.writeTextElement(QStringLiteral("magvar"), QString::number(trkpt->magneticVariation, 'f', 12));

                    // <geoidheight>
                    if (!qIsNaN(trkpt->geoidHeight))
                        xmlWriter.writeTextElement(QStringLiteral("geoidheight"), QString::number(trkpt->geoidHeight, 'f', 12));

                    // <src>
                    if (!trkpt->source.isEmpty())
                        xmlWriter.writeTextElement(QStringLiteral("src"), trkpt->source);

                    // <sym>
                    if (!trkpt->symbol.isEmpty())
                        xmlWriter.writeTextElement(QStringLiteral("sym"), trkpt->symbol);

                    // <fix>
                    switch (trkpt->fixType)
                    {
                        case GpxFixType::None:
                            xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("none"));
                            break;

                        case GpxFixType::PositionOnly:
                            xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("2d"));
                            break;

                        case GpxFixType::PositionAndElevation:
                            xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("3d"));
                            break;

                        case GpxFixType::DGPS:
                            xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("dgps"));
                            break;

                        case GpxFixType::PPS:
                            xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("pps"));
                            break;

                        case GpxFixType::Unknown:
                        default:
                            break;
                    }

                    // <sat>
                    if (trkpt->satellitesUsedForFixCalculation >= 0)
                        xmlWriter.writeTextElement(QStringLiteral("sat"), QString::number(trkpt->satellitesUsedForFixCalculation));

                    // <hdop>
                    if (!qIsNaN(trkpt->horizontalDilutionOfPrecision))
                        xmlWriter.writeTextElement(QStringLiteral("hdop"), QString::number(trkpt->horizontalDilutionOfPrecision, 'f', 12));

                    // <vdop>
                    if (!qIsNaN(trkpt->verticalDilutionOfPrecision))
                        xmlWriter.writeTextElement(QStringLiteral("vdop"), QString::number(trkpt->verticalDilutionOfPrecision, 'f', 12));

                    // <pdop>
                    if (!qIsNaN(trkpt->positionDilutionOfPrecision))
                        xmlWriter.writeTextElement(QStringLiteral("pdop"), QString::number(trkpt->positionDilutionOfPrecision, 'f', 12));

                    // <ageofdgpsdata>
                    if (!qIsNaN(trkpt->ageOfGpsData))
                        xmlWriter.writeTextElement(QStringLiteral("ageofdgpsdata"), QString::number(trkpt->ageOfGpsData, 'f', 12));

                    // <dgpsid>
                    if (trkpt->dgpsStationId >= 0)
                        xmlWriter.writeTextElement(QStringLiteral("dgpsid"), QString::number(trkpt->dgpsStationId));
                }

                // Write extensions
                if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(trackPoint->extraData.shared_ptr()))
                    writeExtensions(extensions, xmlWriter);

                // </trkpt>
                xmlWriter.writeEndElement();
            }

            // Write extensions
            if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(trackSegment->extraData.shared_ptr()))
                writeExtensions(extensions, xmlWriter);

            // </trkseg>
            xmlWriter.writeEndElement();
        }

        // </trk>
        xmlWriter.writeEndElement();
    }

    // <rte>'s
    for (const auto& route : constOf(routes))
    {
        // <rte>
        xmlWriter.writeStartElement(QStringLiteral("rte"));

        // <name>
        if (!route->name.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("name"), route->name);

        // <desc>
        if (!route->description.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("desc"), route->description);

        // <cmt>
        if (!route->comment.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("cmt"), route->comment);

        // <type>
        if (!route->type.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("type"), route->type);

        // Links
        if (!route->links.isEmpty())
            writeLinks(route->links, xmlWriter);

        if (const auto rte = std::dynamic_pointer_cast<const GpxRte>(route.shared_ptr()))
        {
            // <src>
            if (!rte->source.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("src"), rte->source);
        }

        // Write extensions
        if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(route->extraData.shared_ptr()))
            writeExtensions(extensions, xmlWriter);

        // Write route points
        for (const auto& routePoint : constOf(route->points))
        {
            // <rtept>
            xmlWriter.writeStartElement(QStringLiteral("rtept"));
            xmlWriter.writeAttribute(QStringLiteral("lat"), QString::number(routePoint->position.latitude, 'f', 7));
            xmlWriter.writeAttribute(QStringLiteral("lon"), QString::number(routePoint->position.longitude, 'f', 7));

            // <name>
            if (!routePoint->name.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("name"), routePoint->name);

            // <desc>
            if (!routePoint->description.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("desc"), routePoint->description);

            // <ele>
            if (!qIsNaN(routePoint->elevation))
                xmlWriter.writeTextElement(QStringLiteral("ele"), QString::number(routePoint->elevation, 'g', 7));

            // <time>
            if (!routePoint->timestamp.isNull())
                xmlWriter.writeTextElement(QStringLiteral("time"), routePoint->timestamp.toString(Qt::DateFormat::ISODate));

            // <cmt>
            if (!routePoint->comment.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("cmt"), routePoint->comment);

            // <type>
            if (!routePoint->type.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("type"), routePoint->type);

            // Links
            if (!routePoint->links.isEmpty())
                writeLinks(routePoint->links, xmlWriter);

            if (const auto rtept = std::dynamic_pointer_cast<const GpxRtePt>(routePoint.shared_ptr()))
            {
                // <magvar>
                if (!qIsNaN(rtept->magneticVariation))
                    xmlWriter.writeTextElement(QStringLiteral("magvar"), QString::number(rtept->magneticVariation, 'f', 12));

                // <geoidheight>
                if (!qIsNaN(rtept->geoidHeight))
                    xmlWriter.writeTextElement(QStringLiteral("geoidheight"), QString::number(rtept->geoidHeight, 'f', 12));

                // <src>
                if (!rtept->source.isEmpty())
                    xmlWriter.writeTextElement(QStringLiteral("src"), rtept->source);

                // <sym>
                if (!rtept->symbol.isEmpty())
                    xmlWriter.writeTextElement(QStringLiteral("sym"), rtept->symbol);

                // <fix>
                switch (rtept->fixType)
                {
                    case GpxFixType::None:
                        xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("none"));
                        break;

                    case GpxFixType::PositionOnly:
                        xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("2d"));
                        break;

                    case GpxFixType::PositionAndElevation:
                        xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("3d"));
                        break;

                    case GpxFixType::DGPS:
                        xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("dgps"));
                        break;

                    case GpxFixType::PPS:
                        xmlWriter.writeTextElement(QStringLiteral("fix"), QStringLiteral("pps"));
                        break;

                    case GpxFixType::Unknown:
                    default:
                        break;
                }

                // <sat>
                if (rtept->satellitesUsedForFixCalculation >= 0)
                    xmlWriter.writeTextElement(QStringLiteral("sat"), QString::number(rtept->satellitesUsedForFixCalculation));

                // <hdop>
                if (!qIsNaN(rtept->horizontalDilutionOfPrecision))
                    xmlWriter.writeTextElement(QStringLiteral("hdop"), QString::number(rtept->horizontalDilutionOfPrecision, 'f', 12));

                // <vdop>
                if (!qIsNaN(rtept->verticalDilutionOfPrecision))
                    xmlWriter.writeTextElement(QStringLiteral("vdop"), QString::number(rtept->verticalDilutionOfPrecision, 'f', 12));

                // <pdop>
                if (!qIsNaN(rtept->positionDilutionOfPrecision))
                    xmlWriter.writeTextElement(QStringLiteral("pdop"), QString::number(rtept->positionDilutionOfPrecision, 'f', 12));

                // <ageofdgpsdata>
                if (!qIsNaN(rtept->ageOfGpsData))
                    xmlWriter.writeTextElement(QStringLiteral("ageofdgpsdata"), QString::number(rtept->ageOfGpsData, 'f', 12));

                // <dgpsid>
                if (rtept->dgpsStationId >= 0)
                    xmlWriter.writeTextElement(QStringLiteral("dgpsid"), QString::number(rtept->dgpsStationId));
            }

            // Write extensions
            if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(routePoint->extraData.shared_ptr()))
                writeExtensions(extensions, xmlWriter);

            // </rtept>
            xmlWriter.writeEndElement();
        }

        // </rte>
        xmlWriter.writeEndElement();
    }
    // Write gpx extensions
    if (const auto extensions = std::dynamic_pointer_cast<const GpxExtensions>(extraData.shared_ptr()))
        writeExtensions(extensions, xmlWriter);

    // </gpx>
    xmlWriter.writeEndElement();

    xmlWriter.writeEndDocument();

    return true;
}

QString OsmAnd::GpxDocument::getFilename(const QString& path)
{
    QString res;
    if(path.length() > 0)
    {
        int i = path.lastIndexOf('/');
        if(i > 0)
            res = path.mid(i + 1);
        i = res.lastIndexOf('.');
        if(i > 0)
            res = res.mid(0, i);
    }
    return res;
}

void OsmAnd::GpxDocument::writeLinks(const QList< Ref<Link> >& links, QXmlStreamWriter& xmlWriter)
{
    for (const auto& link : links)
    {
        // <link>
        xmlWriter.writeStartElement(QStringLiteral("link"));
        writeNotNullText(xmlWriter, QStringLiteral("href"), link->url.toString());

        // <text>
        if (!link->text.isEmpty())
            xmlWriter.writeTextElement(QStringLiteral("text"), link->text);

        if (const auto gpxLink = std::dynamic_pointer_cast<const GpxLink>(link.shared_ptr()))
        {
            // <type>
            if (!gpxLink->type.isEmpty())
                xmlWriter.writeTextElement(QStringLiteral("type"), gpxLink->type);
        }

        // </link>
        xmlWriter.writeEndElement();
    }
}

void OsmAnd::GpxDocument::writeNotNullTextWithAttribute(QXmlStreamWriter& xmlWriter, const QString& tag, const QString &attribute, const QString& value)
{
    if (!value.isEmpty())
    {
        xmlWriter.writeStartElement(tag);
        xmlWriter.writeTextElement(attribute, value);
        xmlWriter.writeEndElement();
    }
}

void OsmAnd::GpxDocument::writeNotNullText(QXmlStreamWriter& xmlWriter, const QString& tag, const QString& value)
{
    if (!value.isEmpty())
    {
        xmlWriter.writeTextElement(tag, value);
    }
}

void OsmAnd::GpxDocument::writeAuthor(QXmlStreamWriter& xmlWriter, const Ref<Author>& author)
{
    writeNotNullText(xmlWriter, QStringLiteral("name"), author->name);
    if (!author->email.isEmpty() && author->email.contains("@"))
    {
        const auto idAndDomain = author->email.split("@");
        if (idAndDomain.count() == 2 && !idAndDomain[0].isEmpty() && !idAndDomain[1].isEmpty())
        {
            xmlWriter.writeStartElement(QStringLiteral("email"));
            xmlWriter.writeTextElement(QStringLiteral("id"), idAndDomain[0]);
            xmlWriter.writeTextElement(QStringLiteral("domain"), idAndDomain[1]);
            xmlWriter.writeEndElement();
        }
    }
    writeNotNullTextWithAttribute(xmlWriter, QStringLiteral("link"), QStringLiteral("href"), author->link);
}

void OsmAnd::GpxDocument::writeCopyright(QXmlStreamWriter& xmlWriter, const Ref<Copyright>& copyright)
{
    xmlWriter.writeTextElement(QStringLiteral("author"), copyright->author);
    writeNotNullText(xmlWriter, QStringLiteral("year"), copyright->year);
    writeNotNullText(xmlWriter, QStringLiteral("license"), copyright->license);
}

void OsmAnd::GpxDocument::writeBounds(QXmlStreamWriter& xmlWriter, const Ref<Bounds>& bounds)
{
    xmlWriter.writeStartElement(QStringLiteral("bounds"));
    xmlWriter.writeTextElement(QStringLiteral("minlat"), QString::number(bounds->minlat, 'g', 7));
    xmlWriter.writeTextElement(QStringLiteral("minlon"), QString::number(bounds->minlon, 'g', 7));
    xmlWriter.writeTextElement(QStringLiteral("maxlat"), QString::number(bounds->maxlat, 'g', 7));
    xmlWriter.writeTextElement(QStringLiteral("maxlon"), QString::number(bounds->minlat, 'g', 7));
    xmlWriter.writeEndElement();
}

void OsmAnd::GpxDocument::writeExtensions(const std::shared_ptr<const GpxExtensions>& extensions, QXmlStreamWriter& xmlWriter)
{
    if (extensions->extensions.count() == 0)
        return;
    // <extensions>
    xmlWriter.writeStartElement(QStringLiteral("extensions"));
    for (const auto attributeEntry : rangeOf(constOf(extensions->attributes)))
        xmlWriter.writeAttribute(attributeEntry.key(), attributeEntry.value());

    for (const auto& subextension : constOf(extensions->extensions))
        writeExtension(subextension, xmlWriter);

    // </extensions>
    xmlWriter.writeEndElement();
}

void OsmAnd::GpxDocument::writeExtension(const std::shared_ptr<const GpxExtension>& extension, QXmlStreamWriter& xmlWriter)
{
    // <*>
    xmlWriter.writeStartElement(extension->name);
    for (const auto attributeEntry : rangeOf(constOf(extension->attributes)))
        xmlWriter.writeAttribute(attributeEntry.key(), attributeEntry.value());

    if (!extension->value.isEmpty())
        xmlWriter.writeCharacters(extension->value);

    for (const auto& subextension : constOf(extension->subextensions))
        writeExtension(subextension, xmlWriter);

    // </*>
    xmlWriter.writeEndElement();
}

bool OsmAnd::GpxDocument::saveTo(QIODevice& ioDevice, const QString& filename) const
{
    QXmlStreamWriter xmlWriter(&ioDevice);
    xmlWriter.setAutoFormatting(true);
    return saveTo(xmlWriter, filename);
}

bool OsmAnd::GpxDocument::saveTo(const QString& filename) const
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    const bool ok = saveTo(file, filename);
    file.close();

    return ok;
}

std::shared_ptr<OsmAnd::GpxDocument::GpxWpt> OsmAnd::GpxDocument::parseWpt(QXmlStreamReader& xmlReader)
{
    bool ok = true;
    const auto latValue = xmlReader.attributes().value(QStringLiteral("lat"));
    const double lat = latValue.toDouble(&ok);
    if (!ok)
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <wpt> 'lat' attribute value '%s'",
            xmlReader.lineNumber(),
            xmlReader.columnNumber(),
            qPrintableRef(latValue));
        xmlReader.skipCurrentElement();
        return nullptr;
    }
    const auto lonValue = xmlReader.attributes().value(QStringLiteral("lon"));
    const double lon = lonValue.toDouble(&ok);
    if (!ok)
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <wpt> 'lon' attribute value '%s'",
            xmlReader.lineNumber(),
            xmlReader.columnNumber(),
            qPrintableRef(lonValue));
        xmlReader.skipCurrentElement();
        return nullptr;
    }

    auto wpt = std::make_shared<GpxWpt>();
    wpt->position.latitude = lat;
    wpt->position.longitude = lon;
    return wpt;
}

std::shared_ptr<OsmAnd::GpxDocument::GpxTrkPt> OsmAnd::GpxDocument::parseTrkPt(QXmlStreamReader& xmlReader)
{
    bool ok = true;
    const auto latValue = xmlReader.attributes().value(QStringLiteral("lat"));
    const double lat = latValue.toDouble(&ok);
    if (!ok)
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <rtept> 'lat' attribute value '%s'",
            xmlReader.lineNumber(),
            xmlReader.columnNumber(),
            qPrintableRef(latValue));
        xmlReader.skipCurrentElement();
        return nullptr;
    }
    const auto lonValue = xmlReader.attributes().value(QStringLiteral("lon"));
    const double lon = lonValue.toDouble(&ok);
    if (!ok)
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <rtept> 'lon' attribute value '%s'",
            xmlReader.lineNumber(),
            xmlReader.columnNumber(),
            qPrintableRef(lonValue));
        xmlReader.skipCurrentElement();
        return nullptr;
    }

    auto trkpt = std::make_shared<GpxTrkPt>();
    trkpt->position.latitude = lat;
    trkpt->position.longitude = lon;
    return trkpt;
}

std::shared_ptr<OsmAnd::GpxDocument::Bounds> OsmAnd::GpxDocument::parseBoundsAttributes(QXmlStreamReader& xmlReader)
{
    const auto bounds = std::make_shared<Bounds>();
    QString minlat = xmlReader.attributes().value(QStringLiteral("minlat")).toString();
    QString minlon = xmlReader.attributes().value(QStringLiteral("minlon")).toString();
    QString maxlat = xmlReader.attributes().value(QStringLiteral("maxlat")).toString();
    QString maxlon = xmlReader.attributes().value(QStringLiteral("maxlon")).toString();
    
    if (minlat.isEmpty())
        minlat = xmlReader.attributes().value(QStringLiteral("minLat")).toString();
    if (minlon.isEmpty())
        minlon = xmlReader.attributes().value(QStringLiteral("minLon")).toString();
    if (maxlat.isEmpty())
        maxlat = xmlReader.attributes().value(QStringLiteral("maxLat")).toString();
    if (maxlon.isEmpty())
        maxlon = xmlReader.attributes().value(QStringLiteral("maxlon")).toString();
    
    if (!minlat.isEmpty())
        bounds->minlat = minlat.toDouble();
    if (!minlon.isEmpty())
        bounds->minlon = minlon.toDouble();
    if (!maxlat.isEmpty())
        bounds->maxlat = maxlat.toDouble();
    if (!maxlon.isEmpty())
        bounds->maxlon = maxlon.toDouble();
    return bounds;
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::loadFrom(QXmlStreamReader& xmlReader)
{
    std::shared_ptr<GpxDocument> document;
    std::shared_ptr<GpxMetadata> metadata;
    std::shared_ptr<Author> author;
    std::shared_ptr<Copyright> copyright;
    std::shared_ptr<Bounds> bounds;
    std::shared_ptr<GpxWpt> wpt;
    std::shared_ptr<GpxWpt> rpt;
    std::shared_ptr<GpxTrk> trk;
    std::shared_ptr<GpxRte> rte;
    std::shared_ptr<GpxLink> link;
    std::shared_ptr<GpxRtePt> rtept;
    std::shared_ptr<GpxTrkPt> trkpt;
    std::shared_ptr<GpxTrkSeg> trkseg;
    std::shared_ptr<GpxExtensions> extensions;
    QStack< std::shared_ptr<GpxExtension> > extensionStack;

    enum class Token
    {
        gpx,
        metadata,
        copyright,
        author,
        wpt,
        trk,
        rte,
        link,
        rtept,
        trkpt,
        trkseg,
        extensions,
    };
    QStack<Token> tokens;

    auto routeTrack = std::make_shared<GpxTrk>();
    auto routeTrackSegment = std::make_shared<GpxTrkSeg>();
    routeTrack->segments.append(routeTrackSegment);
    bool routePointExtension = false;
    
    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (extensions)
            {
                const std::shared_ptr<GpxExtension> extension(new GpxExtension());
                extension->name = tagName.toString();
                for (const auto& attribute : xmlReader.attributes())
                    extension->attributes[attribute.name().toString()] = attribute.value().toString();

                if (tagName.compare(QStringLiteral("routepointextension"), Qt::CaseInsensitive) == 0)
                {
                    routePointExtension = true;                    
                    if (tokens.top() == Token::rtept || (tokens.size() > 1 && tokens.at(tokens.size() - 2) == Token::rtept))
                        extension->attributes[QStringLiteral("offset")] = QString::number(routeTrackSegment->points.size());
                }

                if (routePointExtension && tagName == QStringLiteral("rpt"))
                {
                    rpt = parseWpt(xmlReader);
                    if (!rpt)
                        continue;
                }
                
                extensionStack.push(extension);
                continue;
            }

            if (tagName == QStringLiteral("gpx"))
            {
                if (document)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): more than one <gpx> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                document.reset(new GpxDocument());
                document->version = xmlReader.attributes().value(QStringLiteral("version")).toString();
                document->creator = xmlReader.attributes().value(QStringLiteral("creator")).toString();

                tokens.push(Token::gpx);
            }
            else if (tagName == QStringLiteral("metadata"))
            {
                if (document->metadata)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): more than one <metadata> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (metadata)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <metadata> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                metadata.reset(new GpxMetadata());
                tokens.push(Token::metadata);

                //TODO:<keywords>
                //TODO:<bounds>
            }
            else if (tagName == QStringLiteral("copyright"))
            {
                if (copyright)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <copyright>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                copyright.reset(new Copyright());
                copyright->license = xmlReader.text().toString();
                copyright->author = xmlReader.attributes().value(QStringLiteral("author")).toString();
                tokens.push(Token::copyright);
            }
            else if (tagName == QStringLiteral("author"))
            {
                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <author> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                const auto name = xmlReader.text().toString();
                switch (tokens.top())
                {
                    case Token::metadata:
                    {
                        author.reset(new Author());
                        author->name = name;
                        tokens.push(Token::author);
                        break;
                    }
                    case Token::copyright:
                        copyright->author = name;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <author> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("wpt"))
            {
                if (wpt)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <wpt>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                wpt = parseWpt(xmlReader);
                if (!wpt)
                    continue;

                tokens.push(Token::wpt);
            }
            else if (tagName == QStringLiteral("trk"))
            {
                if (trk)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <trk>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                trk.reset(new GpxTrk());

                tokens.push(Token::trk);
            }
            else if (tagName == QStringLiteral("rte"))
            {
                if (rte)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <rte>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                rte.reset(new GpxRte());

                tokens.push(Token::rte);
            }
            else if (tagName == QStringLiteral("category"))
            {
                const auto name = xmlReader.readElementText();
                
                if (tokens.isEmpty())
                {
                    LogPrintf(
                              LogSeverityLevel::Warning,
                              "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <category> tag",
                              xmlReader.lineNumber(),
                              xmlReader.columnNumber());
                    continue;
                }
                
                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->category = name;
                        break;
                    case Token::rtept:
                        rtept->category = name;
                        break;
                        
                    default:
                        LogPrintf(
                                  LogSeverityLevel::Warning,
                                  "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <category> tag",
                                  xmlReader.lineNumber(),
                                  xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("email"))
            {
                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <email> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::author:
                    {
                        QString id = xmlReader.attributes().value(QStringLiteral("id")).toString();
                        QString domain = xmlReader.attributes().value(QStringLiteral("domain")).toString();
                        if (!id.isEmpty() && !domain.isEmpty())
                        {
                            author->email = id + QStringLiteral("@") + domain;
                        }
                        break;
                    }

                    default:
                    {
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <email> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                    }
                }
            }
            else if (tagName == QStringLiteral("license"))
            {
                const auto license = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <license> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::copyright:
                    {
                        copyright->license = license;
                        break;
                    }

                    default:
                    {
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <license> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                    }
                }
            }
            else if (tagName == QStringLiteral("year"))
            {
                const auto year = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <year> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::copyright:
                    {
                        copyright->year = year;
                        break;
                    }

                    default:
                    {
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <year> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                    }
                }
            }
            else if (tagName == QStringLiteral("keywords"))
            {
                const auto keywords = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <keywords> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::metadata:
                    {
                        metadata->keywords = keywords;
                        break;
                    }

                    default:
                    {
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <keywords> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                    }
                }
            }
            else if (tagName == QStringLiteral("bounds"))
            {
                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <bounds> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }
                
                auto bounds = parseBoundsAttributes(xmlReader);

                switch (tokens.top())
                {
                    case Token::metadata:
                    {
                        metadata->bounds = bounds;
                        bounds = nullptr;
                        break;
                    }

                    default:
                    {
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <bounds> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                    }
                }
            }
            else if (tagName == QStringLiteral("name"))
            {
                const auto name = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <name> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->name = name;
                        break;
                    case Token::author:
                        author->name = name;
                        break;
                    case Token::wpt:
                        wpt->name = name;
                        break;
                    case Token::trkpt:
                        trkpt->name = name;
                        break;
                    case Token::trk:
                        trk->name = name;
                        break;
                    case Token::rtept:
                        rtept->name = name;
                        break;
                    case Token::rte:
                        rte->name = name;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <name> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("desc"))
            {
                const auto description = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <desc> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->description = description;
                        break;
                    case Token::wpt:
                        wpt->description = description;
                        break;
                    case Token::trkpt:
                        trkpt->description = description;
                        break;
                    case Token::trk:
                        trk->description = description;
                        break;
                    case Token::rtept:
                        rtept->description = description;
                        break;
                    case Token::rte:
                        rte->description = description;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <desc> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("ele"))
            {
                bool ok = false;
                const auto elevationValue = xmlReader.readElementText();
                const auto elevation = elevationValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <ele> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(elevationValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <ele> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->elevation = elevation;
                        break;
                    case Token::trkpt:
                        trkpt->elevation = elevation;
                        break;
                    case Token::rtept:
                        rtept->elevation = elevation;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <ele> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("time"))
            {
                const auto timestampValue = xmlReader.readElementText();
                const auto timestamp = QDateTime::fromString(timestampValue, Qt::DateFormat::ISODate);
                if (!timestamp.isValid() || timestamp.isNull())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <time> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(timestampValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <time> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->timestamp = timestamp;
                        break;
                    case Token::wpt:
                        wpt->timestamp = timestamp;
                        break;
                    case Token::trkpt:
                        trkpt->timestamp = timestamp;
                        break;
                    case Token::rtept:
                        rtept->timestamp = timestamp;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <time> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("magvar"))
            {
                bool ok = false;
                const auto magneticVariationValue = xmlReader.readElementText();
                const auto magneticVariation = magneticVariationValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <magvar> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(magneticVariationValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <magvar> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->magneticVariation = magneticVariation;
                        break;
                    case Token::trkpt:
                        trkpt->magneticVariation = magneticVariation;
                        break;
                    case Token::rtept:
                        rtept->magneticVariation = magneticVariation;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <magvar> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("geoidheight"))
            {
                bool ok = false;
                const auto geoidHeightValue = xmlReader.readElementText();
                const auto geoidHeight = geoidHeightValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <geoidheight> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(geoidHeightValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <geoidheight> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->geoidHeight = geoidHeight;
                        break;
                    case Token::trkpt:
                        trkpt->geoidHeight = geoidHeight;
                        break;
                    case Token::rtept:
                        rtept->geoidHeight = geoidHeight;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <geoidheight> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("cmt"))
            {
                const auto comment = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <cmt> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->comment = comment;
                        break;
                    case Token::trkpt:
                        trkpt->comment = comment;
                        break;
                    case Token::trk:
                        trk->comment = comment;
                        break;
                    case Token::rtept:
                        rtept->comment = comment;
                        break;
                    case Token::rte:
                        rte->comment = comment;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <cmt> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("src"))
            {
                const auto source = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <src> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->source = source;
                        break;
                    case Token::trkpt:
                        trkpt->source = source;
                        break;
                    case Token::trk:
                        trk->source = source;
                        break;
                    case Token::rtept:
                        rtept->source = source;
                        break;
                    case Token::rte:
                        rte->source = source;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <src> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("sym"))
            {
                const auto symbol = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <sym> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->symbol = symbol;
                        break;
                    case Token::trkpt:
                        trkpt->symbol = symbol;
                        break;
                    case Token::rtept:
                        rtept->symbol = symbol;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <sym> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("type"))
            {
                const auto type = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <type> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->type = type;
                        break;
                    case Token::trkpt:
                        trkpt->type = type;
                        break;
                    case Token::trk:
                        trk->type = type;
                        break;
                    case Token::rtept:
                        rtept->type = type;
                        break;
                    case Token::rte:
                        rte->type = type;
                        break;
                    case Token::link:
                        link->type = type;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <type> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("fix"))
            {
                const auto fixValue = xmlReader.readElementText();
                auto fixType = GpxFixType::Unknown;
                if (fixValue == QStringLiteral("none"))
                    fixType = GpxFixType::None;
                else if (fixValue == QStringLiteral("2d"))
                    fixType = GpxFixType::PositionOnly;
                else if (fixValue == QStringLiteral("3d"))
                    fixType = GpxFixType::PositionAndElevation;
                else if (fixValue == QStringLiteral("dgps"))
                    fixType = GpxFixType::DGPS;
                else if (fixValue == QStringLiteral("pps"))
                    fixType = GpxFixType::PPS;
                else
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <fix> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(fixValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <fix> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->fixType = fixType;
                        break;
                    case Token::trkpt:
                        trkpt->fixType = fixType;
                        break;
                    case Token::rtept:
                        rtept->fixType = fixType;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <fix> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("sat"))
            {
                bool ok = false;
                const auto satValue = xmlReader.readElementText();
                const auto satellitesUsedForFixCalculation = satValue.toUInt(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <sat> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(satValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <sat> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->satellitesUsedForFixCalculation = satellitesUsedForFixCalculation;
                        break;
                    case Token::trkpt:
                        trkpt->satellitesUsedForFixCalculation = satellitesUsedForFixCalculation;
                        break;
                    case Token::rtept:
                        rtept->satellitesUsedForFixCalculation = satellitesUsedForFixCalculation;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <sat> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("hdop"))
            {
                bool ok = false;
                const auto hdopValue = xmlReader.readElementText();
                const auto horizontalDilutionOfPrecision = hdopValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <hdop> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(hdopValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <hdop> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->horizontalDilutionOfPrecision = horizontalDilutionOfPrecision;
                        break;
                    case Token::trkpt:
                        trkpt->horizontalDilutionOfPrecision = horizontalDilutionOfPrecision;
                        break;
                    case Token::rtept:
                        rtept->horizontalDilutionOfPrecision = horizontalDilutionOfPrecision;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <hdop> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("vdop"))
            {
                bool ok = false;
                const auto vdopValue = xmlReader.readElementText();
                const auto verticalDilutionOfPrecision = vdopValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <vdop> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(vdopValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <vdop> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->verticalDilutionOfPrecision = verticalDilutionOfPrecision;
                        break;
                    case Token::trkpt:
                        trkpt->verticalDilutionOfPrecision = verticalDilutionOfPrecision;
                        break;
                    case Token::rtept:
                        rtept->verticalDilutionOfPrecision = verticalDilutionOfPrecision;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <vdop> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("pdop"))
            {
                bool ok = false;
                const auto pdopValue = xmlReader.readElementText();
                const auto positionDilutionOfPrecision = pdopValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <pdop> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(pdopValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <pdop> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->positionDilutionOfPrecision = positionDilutionOfPrecision;
                        break;
                    case Token::trkpt:
                        trkpt->positionDilutionOfPrecision = positionDilutionOfPrecision;
                        break;
                    case Token::rtept:
                        rtept->positionDilutionOfPrecision = positionDilutionOfPrecision;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <pdop> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("ageofdgpsdata"))
            {
                bool ok = false;
                const auto ageofdgpsdataValue = xmlReader.readElementText();
                const auto ageOfGpsData = ageofdgpsdataValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <ageofdgpsdata> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(ageofdgpsdataValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <ageofdgpsdata> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->ageOfGpsData = ageOfGpsData;
                        break;
                    case Token::trkpt:
                        trkpt->ageOfGpsData = ageOfGpsData;
                        break;
                    case Token::rtept:
                        rtept->ageOfGpsData = ageOfGpsData;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <ageofdgpsdata> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("dgpsid"))
            {
                bool ok = false;
                const auto dgpsidValue = xmlReader.readElementText();
                const auto dgpsStationId = dgpsidValue.toUInt(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <dgpsid> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(dgpsidValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <dgpsid> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::wpt:
                        wpt->dgpsStationId = dgpsStationId;
                        break;
                    case Token::trkpt:
                        trkpt->dgpsStationId = dgpsStationId;
                        break;
                    case Token::rtept:
                        rtept->dgpsStationId = dgpsStationId;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <dgpsid> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("link"))
            {
                if (link)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <link>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                const auto hrefValue = xmlReader.attributes().value(QStringLiteral("href")).toString();
                const QUrl url(hrefValue);
                if (!url.isValid())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <link> 'href' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(hrefValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                link.reset(new GpxLink());
                link->url = url;

                tokens.push(Token::link);
            }
            else if (tagName == QStringLiteral("text"))
            {
                const auto text = xmlReader.readElementText();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <text> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::link:
                        link->text = text;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <text> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("number"))
            {
                bool ok = false;
                const auto numberValue = xmlReader.readElementText();
                const auto slotNumber = numberValue.toUInt(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <number> value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintable(numberValue));
                    continue;
                }

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <number> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::trk:
                        trk->slotNumber = slotNumber;
                        break;
                    case Token::rte:
                        rte->slotNumber = slotNumber;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <number> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        continue;
                }
            }
            else if (tagName == QStringLiteral("trkpt") || tagName == QStringLiteral("rpt"))
            {
                if (!trkseg && !trk)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): <trkpt> not in <trkseg> and not in <trk>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (trkpt)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <trkpt>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                trkpt = parseTrkPt(xmlReader);
                if (!trkpt)
                    continue;

                tokens.push(Token::trkpt);
            }
            else if (tagName == QStringLiteral("trkseg"))
            {
                if (!trk)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): <trkseg> not in <trk>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (trkseg)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <trkseg>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                trkseg.reset(new GpxTrkSeg());

                tokens.push(Token::trkseg);
            }
            else if (tagName == QStringLiteral("csvattributes"))
            {
                if (!trkseg)
                {
                    LogPrintf(
                              LogSeverityLevel::Warning,
                              "XML warning (%" PRIi64 ", %" PRIi64 "): <trkpt> not in <trkseg>",
                              xmlReader.lineNumber(),
                              xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (trkpt)
                {
                    LogPrintf(
                              LogSeverityLevel::Warning,
                              "XML warning (%" PRIi64 ", %" PRIi64 "): nested <trkpt>",
                              xmlReader.lineNumber(),
                              xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                
                QString points = xmlReader.readElementText();
                QStringList list = points.split(QStringLiteral("\n"), Qt::SkipEmptyParts);

                for (int i = 0; i < list.count(); i++)
                {
                    QStringList coords = list[i].split(QStringLiteral(","), Qt::SkipEmptyParts);
                    int arrLength = coords.count();
                    if (arrLength > 1)
                    {
                        trkpt.reset(new GpxTrkPt());
                        trkpt->position.longitude = coords[0].toDouble();
                        trkpt->position.latitude = coords[1].toDouble();
                        
                        if (arrLength > 2)
                            trkpt->elevation = coords[2].toDouble();
                        
                        trkseg->points.append(trkpt);
                    }
                }
                trkpt = nullptr;
            }
            else if (tagName == QStringLiteral("rtept"))
            {
                if (!rte)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): <rtept> not in <rte>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }
                if (rtept)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): nested <rtept>",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    xmlReader.skipCurrentElement();
                    continue;
                }

                bool ok = true;
                const auto latValue = xmlReader.attributes().value(QStringLiteral("lat"));
                const double lat = latValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <rtept> 'lat' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(latValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }
                const auto lonValue = xmlReader.attributes().value(QStringLiteral("lon"));
                const double lon = lonValue.toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <rtept> 'lon' attribute value '%s'",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber(),
                        qPrintableRef(lonValue));
                    xmlReader.skipCurrentElement();
                    continue;
                }

                rtept.reset(new GpxRtePt());
                rtept->position.latitude = lat;
                rtept->position.longitude = lon;

                tokens.push(Token::rtept);
            }
            else if (tagName == QStringLiteral("extensions"))
            {
                extensions.reset(new GpxExtensions());
                for (const auto& attribute : xmlReader.attributes())
                    extensions->attributes[attribute.name().toString()] = attribute.value().toString();

                tokens.push(Token::extensions);
            }
            else
            {
                LogPrintf(
                    LogSeverityLevel::Warning,
                    "XML warning (%" PRIi64 ", %" PRIi64 "): unknown <%s> tag",
                    xmlReader.lineNumber(),
                    xmlReader.columnNumber(),
                    qPrintableRef(tagName));
                xmlReader.skipCurrentElement();
                continue;
            }
        }
        else if (xmlReader.isEndElement())
        {
            if (tagName.compare(QStringLiteral("routepointextension"), Qt::CaseInsensitive) == 0)
                routePointExtension = false;
            
            if (extensions && !extensionStack.isEmpty())
            {
                if (routePointExtension && tagName == QStringLiteral("rpt"))
                {
                    if (rpt)
                    {
                        routeTrackSegment->points.append(rpt);
                        rpt = nullptr;
                    }
                }
                
                const auto extension = extensionStack.pop();

                if (extensionStack.isEmpty())
                    extensions->extensions.push_back(extension);
                else
                    extensionStack.top()->subextensions.push_back(extension);
                continue;
            }

            if (tagName == QStringLiteral("gpx"))
            {
                tokens.pop();
            }
            else if (tagName == QStringLiteral("metadata"))
            {
                if (document->metadata)
                    continue;

                tokens.pop();

                document->metadata = metadata;
                metadata = nullptr;
            }
            else if (tagName == QStringLiteral("wpt"))
            {
                tokens.pop();

                document->locationMarks.append(wpt);
                wpt = nullptr;
            }
            else if (tagName == QStringLiteral("csvattributes"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("trk"))
            {
                tokens.pop();

                document->tracks.append(trk);
                trk = nullptr;
            }
            else if (tagName == QStringLiteral("rte"))
            {
                document->routes.append(rte);
                rte = nullptr;

                tokens.pop();
            }
            else if (tagName == QStringLiteral("copyright"))
            {
                if (metadata != nullptr)
                    metadata->copyright = copyright;
                
                copyright = nullptr;

                tokens.pop();
            }
            else if (tagName == QStringLiteral("author"))
            {
                switch (tokens.top())
                {
                    case Token::author:
                    {
                        if (metadata != nullptr)
                            metadata->author = author;
                        
                        author = nullptr;

                        tokens.pop();
                    }

                    default:
                        continue;
                }
            }
            else if (tagName == QStringLiteral("keywords"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("bounds"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("license"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("year"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("email"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("name"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("desc"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("ele"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("time"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("magvar"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("geoidheight"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("cmt"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("src"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("sym"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("type"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("fix"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("sat"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("hdop"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("vdop"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("pdop"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("ageofdgpsdata"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("dgpsid"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("link"))
            {
                tokens.pop();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected </link> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::metadata:
                        metadata->links.append(link);
                        link = nullptr;
                        break;
                    case Token::wpt:
                        wpt->links.append(link);
                        link = nullptr;
                        break;
                    case Token::trk:
                        trk->links.append(link);
                        link = nullptr;
                        break;
                    case Token::trkpt:
                        trkpt->links.append(link);
                        link = nullptr;
                        break;
                    case Token::rte:
                        rte->links.append(link);
                        link = nullptr;
                        break;
                    case Token::rtept:
                        rtept->links.append(link);
                        link = nullptr;
                        break;
                    case Token::author:
                        author->link.append(link->url.toString());
                        link = nullptr;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected </link> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        link = nullptr;
                        continue;
                }
            }
            else if (tagName == QStringLiteral("text"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("number"))
            {
                // Do nothing
            }
            else if (tagName == QStringLiteral("trkpt") || tagName == QStringLiteral("rpt"))
            {
                tokens.pop();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected </trkpt> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }
                
                switch (tokens.top())
                {
                    case Token::trk:
                        if (trk->segments.empty())
                            trk->segments.append(std::make_shared<GpxTrkSeg>());

                        trk->segments.last()->points.append(trkpt);
                        break;
                    case Token::trkseg:
                        trkseg->points.append(trkpt);
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected </trkpt> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        trkpt = nullptr;
                        continue;
                }
                
                trkpt = nullptr;
            }
            else if (tagName == QStringLiteral("trkseg"))
            {
                tokens.pop();

                trk->segments.append(trkseg);
                trkseg = nullptr;
            }
            else if (tagName == QStringLiteral("rtept"))
            {
                tokens.pop();

                rte->points.append(rtept);
                rtept = nullptr;
            }
            else if (tagName == QStringLiteral("extensions"))
            {
                tokens.pop();

                if (tokens.isEmpty())
                {
                    LogPrintf(
                        LogSeverityLevel::Warning,
                        "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected <extensions> tag",
                        xmlReader.lineNumber(),
                        xmlReader.columnNumber());
                    continue;
                }

                switch (tokens.top())
                {
                    case Token::gpx:
                        document->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::metadata:
                        metadata->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::wpt:
                        wpt->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::trk:
                        trk->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::trkseg:
                        trkseg->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::trkpt:
                        trkpt->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::rte:
                        rte->extraData = extensions;
                        extensions = nullptr;
                        break;
                    case Token::rtept:
                        rtept->extraData = extensions;
                        extensions = nullptr;
                        break;

                    default:
                        LogPrintf(
                            LogSeverityLevel::Warning,
                            "XML warning (%" PRIi64 ", %" PRIi64 "): unexpected </extensions> tag",
                            xmlReader.lineNumber(),
                            xmlReader.columnNumber());
                        extensions = nullptr;
                        continue;
                }
            }
            else
            {
                LogPrintf(
                    LogSeverityLevel::Warning,
                    "XML warning (%" PRIi64 ", %" PRIi64 "): unknown </%s> tag",
                    xmlReader.lineNumber(),
                    xmlReader.columnNumber(),
                    qPrintableRef(tagName));
                xmlReader.skipCurrentElement();
                continue;
            }
        }
        else if (xmlReader.isCharacters())
        {
            if (extensions)
            {
                if (!extensionStack.isEmpty())
                    extensionStack.top()->value = xmlReader.text().toString();
                else
                    extensions->value = xmlReader.text().toString();
            }
        }
    }
    if (!routeTrackSegment->points.isEmpty()) {
        document->tracks.append(routeTrack);
    }
    if (xmlReader.hasError())
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML error: %s (%" PRIi64 ", %" PRIi64 ")",
            qPrintable(xmlReader.errorString()),
            xmlReader.lineNumber(),
            xmlReader.columnNumber());
        return nullptr;
    }

    return document;
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::loadFrom(QIODevice& ioDevice)
{
    QXmlStreamReader xmlReader(&ioDevice);
    return loadFrom(xmlReader);
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::loadFrom(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return nullptr;
    const auto gpxDocument = loadFrom(file);
    file.close();

    return gpxDocument;
}

OsmAnd::GpxDocument::GpxExtension::GpxExtension()
{
}

OsmAnd::GpxDocument::GpxExtension::~GpxExtension()
{
}

QHash<QString, QVariant> OsmAnd::GpxDocument::GpxExtension::getValues(const bool recursive /*= true*/) const
{
    QHash<QString, QVariant> values;

    if (!value.isEmpty())
        values.insert(name, value);

    const auto prefix = name + QStringLiteral(":");

    for (const auto attributeEntry : rangeOf(constOf(attributes)))
        values.insert(prefix + attributeEntry.key(), attributeEntry.value());

    if (recursive)
    {
        for (const auto& subextension : constOf(subextensions))
        {
            const auto subvalues = subextension->getValues();
            for (const auto attributeEntry : rangeOf(constOf(subvalues)))
                values.insert(prefix + attributeEntry.key(), attributeEntry.value());
        }
    }

    return values;
}

OsmAnd::GpxDocument::GpxExtensions::GpxExtensions()
{
}

OsmAnd::GpxDocument::GpxExtensions::~GpxExtensions()
{
}

QHash<QString, QVariant> OsmAnd::GpxDocument::GpxExtensions::getValues(const bool recursive /*= true*/) const
{
    QHash<QString, QVariant> values;

    if (!value.isEmpty())
        values.insert({}, value);

    for (const auto attributeEntry : rangeOf(constOf(attributes)))
        values.insert(attributeEntry.key(), attributeEntry.value());

    if (recursive)
    {
        for (const auto& subextension : constOf(extensions))
            values.unite(subextension->getValues());
    }

    return values;
}

OsmAnd::GpxDocument::GpxLink::GpxLink()
{
}

OsmAnd::GpxDocument::GpxLink::~GpxLink()
{
}

OsmAnd::GpxDocument::GpxMetadata::GpxMetadata()
{
}

OsmAnd::GpxDocument::GpxMetadata::~GpxMetadata()
{
}

OsmAnd::GpxDocument::GpxWpt::GpxWpt()
    : magneticVariation(std::numeric_limits<double>::quiet_NaN())
    , geoidHeight(std::numeric_limits<double>::quiet_NaN())
    , fixType(GpxFixType::Unknown)
    , satellitesUsedForFixCalculation(-1)
    , horizontalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , verticalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , positionDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
    , ageOfGpsData(std::numeric_limits<double>::quiet_NaN())
    , dgpsStationId(-1)
{
}

OsmAnd::GpxDocument::GpxWpt::~GpxWpt()
{
}

OsmAnd::GpxDocument::GpxTrkPt::GpxTrkPt()
{
}

OsmAnd::GpxDocument::GpxTrkPt::~GpxTrkPt()
{
}

OsmAnd::GpxDocument::GpxTrkSeg::GpxTrkSeg()
{
}

OsmAnd::GpxDocument::GpxTrkSeg::~GpxTrkSeg()
{
}

OsmAnd::GpxDocument::GpxTrk::GpxTrk()
    : slotNumber(-1)
{
}

OsmAnd::GpxDocument::GpxTrk::~GpxTrk()
{
}

OsmAnd::GpxDocument::GpxRtePt::GpxRtePt()
{
}

OsmAnd::GpxDocument::GpxRtePt::~GpxRtePt()
{
}

OsmAnd::GpxDocument::GpxRte::GpxRte()
    : slotNumber(-1)
{
}

OsmAnd::GpxDocument::GpxRte::~GpxRte()
{
}
