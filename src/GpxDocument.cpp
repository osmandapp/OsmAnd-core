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

OsmAnd::GpxExtensions::GpxExtension::GpxExtension()
{
}

OsmAnd::GpxExtensions::GpxExtension::~GpxExtension()
{
}

QHash<QString, QVariant> OsmAnd::GpxExtensions::GpxExtension::getValues(const bool recursive /*= true*/) const
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

OsmAnd::GpxExtensions::GpxExtensions()
{
}

OsmAnd::GpxExtensions::~GpxExtensions()
{
}

QHash<QString, QVariant> OsmAnd::GpxExtensions::getValues(const bool recursive /*= true*/) const
{
    QHash<QString, QVariant> values;

    if (!value.isEmpty())
        values.insert(QString(), value);

    for (const auto attributeEntry : rangeOf(constOf(attributes)))
        values.insert(attributeEntry.key(), attributeEntry.value());

    if (recursive)
    {
        for (const auto& subextension : constOf(extensions))
            values.unite(subextension->getValues());
    }

    return values;
}

OsmAnd::GpxDocument::GpxDocument()
{
}

OsmAnd::GpxDocument::~GpxDocument()
{
}

bool OsmAnd::GpxDocument::hasRtePt() const
{
    for (auto& r : routes)
        if (r->points.size() > 0)
            return true;

    return false;
}

bool OsmAnd::GpxDocument::hasWptPt() const
{
    return points.size() > 0;
}

bool OsmAnd::GpxDocument::hasTrkPt() const
{
    for (auto& t : tracks)
        for (auto& ts : t->segments)
            if (ts->points.size() > 0)
                return true;

    return false;
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::createFrom(const std::shared_ptr<const GpxDocument>& document)
{
    return nullptr;
}

bool OsmAnd::GpxDocument::saveTo(QXmlStreamWriter& xmlWriter, const QString& filename) const
{
    xmlWriter.writeStartDocument(QStringLiteral("1.0"), true);

    //<gpx
    //      version="1.1"
    //      creator="OsmAnd"
    //      xmlns="http://www.topografix.com/GPX/1/1"
    //      xmlns:osmand="https://osmand.net"
    //      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    //      xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
    xmlWriter.writeStartElement(QStringLiteral("gpx"));
    xmlWriter.writeAttribute(QStringLiteral("version"), version.isEmpty() ? QStringLiteral("1.1") : version);
    xmlWriter.writeAttribute(QStringLiteral("creator"), creator.isEmpty() ? QStringLiteral("OsmAnd Core") : creator);
    xmlWriter.writeAttribute(QStringLiteral("xmlns"), QStringLiteral("http://www.topografix.com/GPX/1/1"));
    xmlWriter.writeAttribute(QStringLiteral("xmlns:osmand"), QStringLiteral("https://osmand.net"));
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

        writeNotNullText(xmlWriter, QStringLiteral("keywords"), metadata->keywords);
        if (metadata->bounds)
            writeBounds(xmlWriter, metadata->bounds);

        // Links
        if (!metadata->links.isEmpty())
            writeLinks(metadata->links, xmlWriter);

        // <time>
        if (!metadata->timestamp.isNull())
            xmlWriter.writeTextElement(QStringLiteral("time"), metadata->timestamp.toString(Qt::DateFormat::ISODate));

        // Write extensions
        writeExtensions(metadata->extensions, metadata->attributes, xmlWriter);
    }
    // </metadata>
    xmlWriter.writeEndElement();

    // <wpt>'s
    for (const auto& wptPt : constOf(points))
    {
        // <wpt>
        xmlWriter.writeStartElement(QStringLiteral("wpt"));
        xmlWriter.writeAttribute(QStringLiteral("lat"), QString::number(wptPt->position.latitude, 'f', 7));
        xmlWriter.writeAttribute(QStringLiteral("lon"), QString::number(wptPt->position.longitude, 'f', 7));

        // <name>
        writeNotNullText(xmlWriter, QStringLiteral("name"), wptPt->name);

        // <desc>
        writeNotNullText(xmlWriter, QStringLiteral("desc"), wptPt->description);

        // <ele>
        if (!qIsNaN(wptPt->elevation))
            xmlWriter.writeTextElement(QStringLiteral("ele"), QString::number(wptPt->elevation, 'g', 7));

        // <time>
        if (!wptPt->timestamp.isNull())
            xmlWriter.writeTextElement(QStringLiteral("time"), wptPt->timestamp.toString(Qt::DateFormat::ISODate));

        // <cmt>
        writeNotNullText(xmlWriter, QStringLiteral("cmt"), wptPt->comment);

        // <type>
        writeNotNullText(xmlWriter, QStringLiteral("type"), wptPt->type);

        // Links
        if (!wptPt->links.isEmpty())
            writeLinks(wptPt->links, xmlWriter);

        if (const auto wpt = std::dynamic_pointer_cast<const WptPt>(wptPt.shared_ptr()))
        {
            // <hdop>
            if (!qIsNaN(wpt->horizontalDilutionOfPrecision))
                xmlWriter.writeTextElement(QStringLiteral("hdop"), QString::number(wpt->horizontalDilutionOfPrecision, 'f', 7));

            // <vdop>
            if (!qIsNaN(wpt->verticalDilutionOfPrecision))
                xmlWriter.writeTextElement(QStringLiteral("vdop"), QString::number(wpt->verticalDilutionOfPrecision, 'f', 12));
        }
        

        // Write extensions
        writeExtensions(wptPt->extensions, wptPt->attributes, xmlWriter);

        // </wpt>
        xmlWriter.writeEndElement();
    }

    // <trk>'s
    for (const auto& track : constOf(tracks))
    {
        // <trk>
        xmlWriter.writeStartElement(QStringLiteral("trk"));

        // <name>
        writeNotNullText(xmlWriter, QStringLiteral("name"), track->name);

        // <desc>
        writeNotNullText(xmlWriter, QStringLiteral("desc"), track->description);

        // Write extensions
        writeExtensions(track->extensions, track->attributes, xmlWriter);

        // Write track segments
        for (const auto& trackSegment : constOf(track->segments))
        {
            // <trkseg>
            xmlWriter.writeStartElement(QStringLiteral("trkseg"));

            // <name>
            writeNotNullText(xmlWriter, QStringLiteral("name"), trackSegment->name);

            // Write track points
            for (const auto& trackPoint : constOf(trackSegment->points))
            {
                // <trkpt>
                xmlWriter.writeStartElement(QStringLiteral("trkpt"));
                xmlWriter.writeAttribute(QStringLiteral("lat"), QString::number(trackPoint->position.latitude, 'f', 7));
                xmlWriter.writeAttribute(QStringLiteral("lon"), QString::number(trackPoint->position.longitude, 'f', 7));

                // <name>
                writeNotNullText(xmlWriter, QStringLiteral("name"), trackPoint->name);

                // <desc>
                writeNotNullText(xmlWriter, QStringLiteral("desc"), trackPoint->description);

                // <ele>
                if (!qIsNaN(trackPoint->elevation))
                    xmlWriter.writeTextElement(QStringLiteral("ele"), QString::number(trackPoint->elevation, 'g', 7));

                // <time>
                if (!trackPoint->timestamp.isNull())
                    xmlWriter.writeTextElement(QStringLiteral("time"), trackPoint->timestamp.toString(Qt::DateFormat::ISODate));

                // <cmt>
                writeNotNullText(xmlWriter, QStringLiteral("cmt"), trackPoint->comment);

                // <type>
                writeNotNullText(xmlWriter, QStringLiteral("type"), trackPoint->type);

                // Links
                if (!trackPoint->links.isEmpty())
                    writeLinks(trackPoint->links, xmlWriter);

                if (auto trkpt = std::const_pointer_cast<WptPt>(trackPoint.shared_ptr()))
                {
                    // <hdop>
                    if (!qIsNaN(trkpt->horizontalDilutionOfPrecision))
                        xmlWriter.writeTextElement(QStringLiteral("hdop"), QString::number(trkpt->horizontalDilutionOfPrecision, 'f', 7));

                    // <vdop>
                    if (!qIsNaN(trkpt->verticalDilutionOfPrecision))
                        xmlWriter.writeTextElement(QStringLiteral("vdop"), QString::number(trkpt->verticalDilutionOfPrecision, 'f', 7));
                    
                    // TODO: refactor extensions to be Map<String, String> as in android and get rid of this code
                    auto it = trkpt->extensions.begin();
                    while (it != trkpt->extensions.end())
                    {
                        if ((*it)->name == QStringLiteral("speed") || (*it)->name == QStringLiteral("heading"))
                            it = trkpt->extensions.erase(it);
                        else
                            ++it;
                    }
                    
                    if (trkpt->speed > 0)
                    {
                        const auto extension = std::make_shared<GpxExtensions::GpxExtension>();
                        extension->name = QStringLiteral("speed");
                        extension->value = QString::number(trkpt->speed, 'f', 1);
                        trkpt->extensions.append(extension);
                    }
                    if (!isnan(trkpt->heading))
                    {
                        const auto extension = std::make_shared<GpxExtensions::GpxExtension>();
                        extension->name = QStringLiteral("heading");
                        extension->value = QString::number(round(trkpt->heading), 'f', 0);
                        trkpt->extensions.append(extension);
                    }
                }

                // Write extensions
                writeExtensions(trackPoint->extensions, trackPoint->attributes, xmlWriter);

                // </trkpt>
                xmlWriter.writeEndElement();
            }

            // Write extensions
            writeExtensions(trackSegment->extensions, trackSegment->attributes, xmlWriter);

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
        writeNotNullText(xmlWriter, QStringLiteral("name"), route->name);

        // <desc>
        writeNotNullText(xmlWriter, QStringLiteral("desc"), route->description);

        // Write extensions
        writeExtensions(route->extensions, route->attributes, xmlWriter);

        // Write route points
        for (const auto& routePoint : constOf(route->points))
        {
            // <rtept>
            xmlWriter.writeStartElement(QStringLiteral("rtept"));
            xmlWriter.writeAttribute(QStringLiteral("lat"), QString::number(routePoint->position.latitude, 'f', 7));
            xmlWriter.writeAttribute(QStringLiteral("lon"), QString::number(routePoint->position.longitude, 'f', 7));

            // <name>
            writeNotNullText(xmlWriter, QStringLiteral("name"), routePoint->name);

            // <desc>
            writeNotNullText(xmlWriter, QStringLiteral("desc"), routePoint->description);

            // <ele>
            if (!qIsNaN(routePoint->elevation))
                xmlWriter.writeTextElement(QStringLiteral("ele"), QString::number(routePoint->elevation, 'g', 7));

            // <time>
            if (!routePoint->timestamp.isNull())
                xmlWriter.writeTextElement(QStringLiteral("time"), routePoint->timestamp.toString(Qt::DateFormat::ISODate));

            // <cmt>
            writeNotNullText(xmlWriter, QStringLiteral("cmt"), routePoint->comment);

            // <type>
            writeNotNullText(xmlWriter, QStringLiteral("type"), routePoint->type);

            // Links
            if (!routePoint->links.isEmpty())
                writeLinks(routePoint->links, xmlWriter);

            if (const auto rtept = std::dynamic_pointer_cast<const WptPt>(routePoint.shared_ptr()))
            {
                // <hdop>
                if (!qIsNaN(rtept->horizontalDilutionOfPrecision))
                    xmlWriter.writeTextElement(QStringLiteral("hdop"), QString::number(rtept->horizontalDilutionOfPrecision, 'f', 12));

                // <vdop>
                if (!qIsNaN(rtept->verticalDilutionOfPrecision))
                    xmlWriter.writeTextElement(QStringLiteral("vdop"), QString::number(rtept->verticalDilutionOfPrecision, 'f', 12));
            }

            // Write extensions
            writeExtensions(routePoint->extensions, routePoint->attributes, xmlWriter);

            // </rtept>
            xmlWriter.writeEndElement();
        }

        // </rte>
        xmlWriter.writeEndElement();
    }

    // Write gpx extensions
    writeExtensions(extensions, attributes, xmlWriter);

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

void OsmAnd::GpxDocument::writeExtensions(const QList< Ref<GpxExtension> > &extensions, const QHash<QString, QString> &attributes, QXmlStreamWriter& xmlWriter)
{
    if (extensions.count() == 0)
        return;
    // <extensions>
    xmlWriter.writeStartElement(QStringLiteral("extensions"));
    for (const auto attributeEntry : rangeOf(constOf(attributes)))
        xmlWriter.writeAttribute(attributeEntry.key(), attributeEntry.value());

    for (const auto& subextension : constOf(extensions))
        writeExtension(subextension, xmlWriter, QStringLiteral("osmand"));

    // </extensions>
    xmlWriter.writeEndElement();
}

void OsmAnd::GpxDocument::writeExtension(const std::shared_ptr<const GpxExtension>& extension, QXmlStreamWriter& xmlWriter, const QString &namesp)
{
    // <*>
    xmlWriter.writeStartElement(namesp, extension->name);
    for (const auto attributeEntry : rangeOf(constOf(extension->attributes)))
        xmlWriter.writeAttribute(attributeEntry.key(), attributeEntry.value());

    if (!extension->value.isEmpty())
        xmlWriter.writeCharacters(extension->value);

    for (const auto& subextension : constOf(extension->subextensions))
        writeExtension(subextension, xmlWriter, QStringLiteral(""));

    // assignRouteExtensionWriter(segment);
    // </*>
    xmlWriter.writeEndElement();
}

bool OsmAnd::GpxDocument::saveTo(QIODevice& ioDevice, const QString& filename) const
{
    QXmlStreamWriter xmlWriter(&ioDevice);
    xmlWriter.setAutoFormatting(true);
    xmlWriter.writeNamespace(QStringLiteral("osmand"), QStringLiteral("osmand"));
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

std::shared_ptr<OsmAnd::GpxDocument::WptPt> OsmAnd::GpxDocument::parseWptAttributes(QXmlStreamReader& xmlReader)
{
    auto wpt = std::make_shared<WptPt>();
    wpt->speed = 0;

    bool ok = true;
    const auto latValue = xmlReader.attributes().value(QStringLiteral("lat"));
    const double lat = latValue.toDouble(&ok);
    if (ok)
    {
        wpt->position.latitude = lat;
    }
    else
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <wpt> 'lat' attribute value '%s'",
            xmlReader.lineNumber(),
            xmlReader.columnNumber(),
            qPrintableRef(latValue));
    }

    const auto lonValue = xmlReader.attributes().value(QStringLiteral("lon"));
    const double lon = lonValue.toDouble(&ok);
    if (ok)
    {
        wpt->position.longitude = lon;
    }
    else
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML warning (%" PRIi64 ", %" PRIi64 "): invalid <wpt> 'lon' attribute value '%s'",
            xmlReader.lineNumber(),
            xmlReader.columnNumber(),
            qPrintableRef(lonValue));
    }

    return wpt;
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

std::shared_ptr<OsmAnd::GpxDocument::RouteSegment> OsmAnd::GpxDocument::parseRouteSegmentAttributes(QXmlStreamReader& parser)
{
    const auto segment = std::make_shared<RouteSegment>();
    segment->id = parser.attributes().value(QStringLiteral(""), QStringLiteral("id")).toString();
    segment->length = parser.attributes().value(QStringLiteral(""), QStringLiteral("length")).toString();
    segment->segmentTime = parser.attributes().value(QStringLiteral(""), QStringLiteral("segmentTime")).toString();
    segment->speed = parser.attributes().value(QStringLiteral(""), QStringLiteral("speed")).toString();
    segment->turnType = parser.attributes().value(QStringLiteral(""), QStringLiteral("turnType")).toString();
    segment->turnAngle = parser.attributes().value(QStringLiteral(""), QStringLiteral("turnAngle")).toString();
    segment->types = parser.attributes().value(QStringLiteral(""), QStringLiteral("types")).toString();
    segment->pointTypes = parser.attributes().value(QStringLiteral(""), QStringLiteral("pointTypes")).toString();
    segment->names = parser.attributes().value(QStringLiteral(""), QStringLiteral("names")).toString();
    return segment;
}

std::shared_ptr<OsmAnd::GpxDocument::RouteType> OsmAnd::GpxDocument::parseRouteTypeAttributes(QXmlStreamReader& parser)
{
    const auto type = std::make_shared<RouteType>();
    type->tag = parser.attributes().value(QStringLiteral(""), QStringLiteral("t")).toString();
    type->value = parser.attributes().value(QStringLiteral(""), QStringLiteral("v")).toString();
    return type;
}

QString OsmAnd::GpxDocument::readText(QXmlStreamReader& xmlReader, QString key)
{
    QXmlStreamReader::TokenType tok;
    QString text;
    while ((tok = xmlReader.readNext()) != QXmlStreamReader::TokenType::EndDocument)
    {
        if (tok == QXmlStreamReader::TokenType::EndElement && xmlReader.name().toString() == key)
        {
            break;
        }
        else if (tok == QXmlStreamReader::TokenType::Characters)
        {
            if (text.isNull())
                text = xmlReader.text().toString();
            else
                text.append(xmlReader.text().toString());
        }
    }
    return text;
}

QMap<QString, QString> OsmAnd::GpxDocument::readTextMap(QXmlStreamReader& xmlReader, QString key)
{
    QXmlStreamReader::TokenType tok;
    QString text;
    QMap<QString, QString> result;
    while ((tok = xmlReader.readNext()) != QXmlStreamReader::TokenType::EndDocument)
    {
        if (tok == QXmlStreamReader::TokenType::EndElement)
        {
            QString tag = xmlReader.name().toString();
            if (!text.isNull() && !text.trimmed().isEmpty())
                result.insert(tag, text);
            if (tag == key)
                break;
            text = QStringLiteral("");
        }
        else if (tok == QXmlStreamReader::TokenType::StartElement)
        {
            text = QStringLiteral("");
        }
        else if (tok == QXmlStreamReader::TokenType::Characters)
        {
            if (text.isEmpty())
                text = xmlReader.text().toString();
            else
                text.append(xmlReader.text().toString());
        }
    }
    return result;
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::GpxDocument::loadFrom(QXmlStreamReader& parser)
{
    auto routeTrack = std::make_shared<Track>();
    auto routeTrackSegment = std::make_shared<TrkSegment>();
    routeTrack->segments.append(routeTrackSegment);
    QStack<std::shared_ptr<GpxExtensions>> parserState;
    std::shared_ptr<TrkSegment> firstSegment;
    bool extensionReadMode = false;
    bool routePointExtension = false;
    QList< Ref<RouteSegment> > routeSegments;
    QList< Ref<RouteType> > routeTypes;
    bool routeExtension = false;
    bool typesExtension = false;
    auto document = std::make_shared<GpxDocument>();
    parserState.push(document);

    QMap<QString, QString> gpxTypes;
    gpxTypes.insert(QStringLiteral("gpx"), QString(typeid(GpxDocument).name()));
    gpxTypes.insert(QStringLiteral("metadata"), QString(typeid(Metadata).name()));
    gpxTypes.insert(QStringLiteral("author"), QString(typeid(Author).name()));
    gpxTypes.insert(QStringLiteral("copyright"), QString(typeid(Copyright).name()));
    gpxTypes.insert(QStringLiteral("bounds"), QString(typeid(Bounds).name()));
    gpxTypes.insert(QStringLiteral("wpt"), QString(typeid(WptPt).name()));
    gpxTypes.insert(QStringLiteral("rte"), QString(typeid(Route).name()));
    gpxTypes.insert(QStringLiteral("rtept"), QString(typeid(WptPt).name()));
    gpxTypes.insert(QStringLiteral("trk"), QString(typeid(Track).name()));
    gpxTypes.insert(QStringLiteral("trkseg"), QString(typeid(TrkSegment).name()));
    gpxTypes.insert(QStringLiteral("trkpt"), QString(typeid(WptPt).name()));
    gpxTypes.insert(QStringLiteral("rpt"), QString(typeid(WptPt).name()));

    QXmlStreamReader::TokenType tok;
    while ((tok = parser.readNext()) != QXmlStreamReader::TokenType::EndDocument)
    {
        if (tok == QXmlStreamReader::TokenType::StartElement)
        {
            std::shared_ptr<GpxExtensions> parse = parserState.top();
            QString tag = parser.name().toString();
            QString parseClassName = QString(typeid(*parse).name());
            if (gpxTypes.keys().contains(tag) && gpxTypes.value(tag) != parseClassName)
                extensionReadMode = false;

            if (extensionReadMode && parse != nullptr && !routePointExtension)
            {
                QString tagName = tag.toLower();
                if (routeExtension)
                {
                    if (tagName == QStringLiteral("segment"))
                    {
                        std::shared_ptr<RouteSegment> segment = GpxDocument::parseRouteSegmentAttributes(parser);
                        routeSegments.append(segment);
                    }
                }
                else if (typesExtension)
                {
                    if (tagName == QStringLiteral("type"))
                    {
                        std::shared_ptr<RouteType> type = GpxDocument::parseRouteTypeAttributes(parser);
                        routeTypes.append(type);
                    }
                }
                if (tagName == QStringLiteral("routepointextension"))
                {
                    routePointExtension = true;
                    if (auto wptPt = std::dynamic_pointer_cast<WptPt>(parse))
                    {
                        const auto extension = std::make_shared<GpxExtension>();
                        extension->name = QStringLiteral("offset");
                        extension->value = QString::number(routeTrackSegment->points.size());
                        wptPt->extensions.append(extension);
                    }
                }
                else if (tagName == QStringLiteral("route"))
                {
                    routeExtension = true;
                }
                else if (tagName == QStringLiteral("types"))
                {
                    typesExtension = true;
                }
                else
                {
                    QMap<QString, QString> values = GpxDocument::readTextMap(parser, tag);
                    if (values.size() > 0)
                    {
                        QList<QString> keys = values.keys();
                        for (QString key : keys)
                        {
                            QString t = key.toLower();
                            QString value = values[key];
                            const auto extension = std::make_shared<GpxExtensions::GpxExtension>();
                            extension->name = t;
                            extension->value = value;
                            parse->extensions.append(extension);

                            if (tag == QStringLiteral("speed"))
                            {
                                if (auto wptPt = std::dynamic_pointer_cast<WptPt>(parse))
                                {
                                    bool ok = true;
                                    auto speed{ value.toDouble(&ok) };
                                    if (ok)
                                        wptPt->speed = speed;
                                }
                            }
                            else if (tag == QStringLiteral("heading"))
                            {
                                if (auto wptPt = std::dynamic_pointer_cast<WptPt>(parse))
                                {
                                    bool ok = true;
                                    auto heading{ value.toDouble(&ok) };
                                    if (ok)
                                        wptPt->heading = heading;
                                }
                            }
                        }
                    }
                }
            }
            else if (parse != nullptr && tag == QStringLiteral("extensions"))
            {
                extensionReadMode = true;
            }
            else if (routePointExtension)
            {
                if (tag == QStringLiteral("rpt"))
                {
                    std::shared_ptr<WptPt> wptPt = GpxDocument::parseWptAttributes(parser);
                    routeTrackSegment->points.append(wptPt);
                    parserState.push(wptPt);
                }
            }
            else
            {
                if (auto gpxDocument = std::dynamic_pointer_cast<GpxDocument>(parse))
                {
                    if (tag == QStringLiteral("gpx"))
                    {
                        gpxDocument->creator = parser.attributes().value(QStringLiteral("creator")).toString();
                        gpxDocument->version = parser.attributes().value(QStringLiteral("version")).toString();
                    }
                    if (tag == QStringLiteral("metadata"))
                    {
                        const auto metadata = std::make_shared<Metadata>();
                        gpxDocument->metadata = metadata;
                        parserState.push(metadata);
                    }
                    if (tag == QStringLiteral("trk"))
                    {
                        const auto track = std::make_shared<Track>();
                        gpxDocument->tracks.append(track);
                        parserState.push(track);
                    }
                    if (tag == QStringLiteral("rte"))
                    {
                        const auto route = std::make_shared<Route>();
                        gpxDocument->routes.append(route);
                        parserState.push(route);
                    }
                    if (tag == QStringLiteral("wpt"))
                    {
                        std::shared_ptr<WptPt> wptPt = GpxDocument::parseWptAttributes(parser);
                        gpxDocument->points.append(wptPt);
                        parserState.push(wptPt);
                    }
                }
                else if (auto metadata = std::dynamic_pointer_cast<Metadata>(parse))
                {
                    if (tag == QStringLiteral("name"))
                        metadata->name = GpxDocument::readText(parser, QStringLiteral("name"));
                    if (tag == QStringLiteral("desc"))
                        metadata->description = GpxDocument::readText(parser, QStringLiteral("desc"));
                    if (tag == QStringLiteral("author"))
                    {
                        const auto author = std::make_shared<Author>();
                        author->name = parser.text().toString();
                        metadata->author = author;
                        parserState.push(author);
                    }
                    if (tag == QStringLiteral("copyright"))
                    {
                        const auto copyright = std::make_shared<Copyright>();
                        copyright->license = parser.text().toString();
                        copyright->author = parser.attributes().value(QStringLiteral(""), QStringLiteral("author")).toString();
                        metadata->copyright = copyright;
                        parserState.push(copyright);
                    }
                    if (tag == QStringLiteral("link"))
                    {
                        const auto link = std::make_shared<Link>();
                        link->url = parser.attributes().value(QStringLiteral(""), QStringLiteral("href")).toString();
                        metadata->links.append(link);
                    }
                    if (tag == QStringLiteral("time"))
                    {
                        QString text = GpxDocument::readText(parser, QStringLiteral("time"));
                        const auto timestamp = QDateTime::fromString(text, Qt::DateFormat::ISODate);
                        if (timestamp.isValid() && !timestamp.isNull())
                            metadata->timestamp = timestamp;
                    }
                    if (tag == QStringLiteral("keywords"))
                        metadata->keywords = GpxDocument::readText(parser, QStringLiteral("keywords"));
                    if (tag == QStringLiteral("bounds"))
                    {
                        std::shared_ptr<Bounds> bounds = GpxDocument::parseBoundsAttributes(parser);
                        metadata->bounds = bounds;
                        parserState.push(bounds);
                    }
                }
                else if (auto author = std::dynamic_pointer_cast<Author>(parse))
                {
                    if (tag == QStringLiteral("name"))
                        author->name = GpxDocument::readText(parser, QStringLiteral("name"));
                    if (tag == QStringLiteral("email"))
                    {
                        QString id = parser.attributes().value(QStringLiteral(""), QStringLiteral("id")).toString();
                        QString domain = parser.attributes().value(QStringLiteral(""), QStringLiteral("domain")).toString();
                        if (!id.isNull() && !id.isEmpty() && !domain.isNull() && !domain.isEmpty())
                        {
                            author->email = id.append(QStringLiteral("@")).append(domain);
                        }
                    }
                    if (tag == QStringLiteral("link"))
                        author->link = parser.attributes().value(QStringLiteral(""), QStringLiteral("href")).toString();
                }
                else if (auto copyright = std::dynamic_pointer_cast<Copyright>(parse))
                {
                    if (tag == QStringLiteral("year"))
                        copyright->year = GpxDocument::readText(parser, QStringLiteral("year"));
                    if (tag == QStringLiteral("license"))
                        copyright->license = GpxDocument::readText(parser, QStringLiteral("license"));
                }
                else if (auto route = std::dynamic_pointer_cast<Route>(parse))
                {
                    if (tag == QStringLiteral("name"))
                    {
                        route->name = GpxDocument::readText(parser, QStringLiteral("name"));
                    }
                    if (tag == QStringLiteral("desc"))
                    {
                        route->description = GpxDocument::readText(parser, QStringLiteral("desc"));
                    }
                    if (tag == QStringLiteral("rtept"))
                    {
                        std::shared_ptr<WptPt> wptPt = GpxDocument::parseWptAttributes(parser);
                        route->points.append(wptPt);
                        parserState.push(wptPt);
                    }
                }
                else if (auto track = std::dynamic_pointer_cast<Track>(parse))
                {
                    if (tag == QStringLiteral("name"))
                    {
                        track->name = GpxDocument::readText(parser, "name");
                    }
                    else if (tag == QStringLiteral("desc"))
                    {
                        track->description = GpxDocument::readText(parser, "desc");
                    }
                    else if (tag == QStringLiteral("trkseg"))
                    {
                        const auto trkSeg = std::make_shared<TrkSegment>();
                        track->segments.append(trkSeg);
                        parserState.push(trkSeg);
                    }
                    else if (tag == QStringLiteral("trkpt") || tag == QStringLiteral("rpt"))
                    {
                        std::shared_ptr<WptPt> wptPt = GpxDocument::parseWptAttributes(parser);
                        int size = track->segments.size();
                        if (size == 0)
                        {
                            const auto trkSeg = std::make_shared<TrkSegment>();
                            track->segments.append(trkSeg);
                            size++;
                        }
                        track->segments[size - 1]->points.append(wptPt);
                        parserState.push(wptPt);
                    }
                }
                else if (auto segment = std::dynamic_pointer_cast<TrkSegment>(parse))
                {
                    if (tag == QStringLiteral("name"))
                    {
                        segment->name = GpxDocument::readText(parser, QStringLiteral("name"));
                    }
                    else if (tag == QStringLiteral("trkpt") || tag == QStringLiteral("rpt"))
                    {
                        std::shared_ptr<WptPt> wptPt = GpxDocument::parseWptAttributes(parser);
                        segment->points.append(wptPt);
                        parserState.push(wptPt);
                    }
                    if (tag == QStringLiteral("csvattributes"))
                    {
                        QString segmentPoints = readText(parser, QStringLiteral("csvattributes"));
                        QStringList pointsArr = segmentPoints.split("\n");
                        for (int i = 0; i < pointsArr.size(); i++)
                        {
                            QStringList pointAttrs = pointsArr[i].split(",");
                            int arrLength = pointsArr.length();
                            if (arrLength > 1)
                            {
                                const auto wptPt = std::make_shared<WptPt>();
                                bool ok = true;
                                auto longitude{ pointAttrs[0].toDouble(&ok) };
                                if (ok)
                                    wptPt->position.longitude = longitude;

                                auto latitude{ pointAttrs[1].toDouble(&ok) };
                                if (ok)
                                    wptPt->position.latitude = latitude;

                                segment->points.append(wptPt);
                                if (arrLength > 2)
                                {
                                    auto elevation{ pointAttrs[2].toDouble(&ok) };
                                    if (ok)
                                        wptPt->elevation = elevation;
                                }
                            }
                        }
                    }
                    // main object to parse
                }
                else if (auto wptPt = std::dynamic_pointer_cast<WptPt>(parse))
                {
                    if (tag == QStringLiteral("name"))
                    {
                        wptPt->name = GpxDocument::readText(parser, QStringLiteral("name"));
                    }
                    else if (tag == QStringLiteral("desc"))
                    {
                        wptPt->description = GpxDocument::readText(parser, QStringLiteral("desc"));
                    }
                    else if (tag == QStringLiteral("cmt"))
                    {
                        wptPt->comment = GpxDocument::readText(parser, QStringLiteral("cmt"));
                    }
                    else if (tag == QStringLiteral("speed"))
                    {
                        QString value = GpxDocument::readText(parser, QStringLiteral("speed"));
                        if (!value.isNull() && !value.isEmpty())
                        {
                            bool ok = true;
                            auto speed{ value.toDouble(&ok) };
                            if (ok)
                                wptPt->speed = speed;
                        }
                    }
                    else if (tag == QStringLiteral("link"))
                    {
                        const auto link = std::make_shared<Link>();
                        link->url = parser.attributes().value(QStringLiteral(""), QStringLiteral("href")).toString();
                        wptPt->links.append(link);
                    }
                    else if (tag == QStringLiteral("category"))
                    {
                        wptPt->type = GpxDocument::readText(parser, QStringLiteral("category"));
                    }
                    else if (tag == QStringLiteral("type"))
                    {
                        if (wptPt->type.isEmpty())
                            wptPt->type = GpxDocument::readText(parser, QStringLiteral("type"));
                    }
                    else if (tag == QStringLiteral("ele"))
                    {
                        QString text = GpxDocument::readText(parser, QStringLiteral("ele"));
                        if (!text.isNull() && !text.isEmpty())
                        {
                            bool ok = true;
                            auto elevation{ text.toDouble(&ok) };
                            if (ok)
                                wptPt->elevation = elevation;
                        }
                    }
                    else if (tag == QStringLiteral("hdop"))
                    {
                        QString text = GpxDocument::readText(parser, QStringLiteral("hdop"));
                        if (!text.isNull() && !text.isEmpty())
                        {
                            bool ok = true;
                            auto hdop{ text.toDouble(&ok) };
                            if (ok)
                                wptPt->horizontalDilutionOfPrecision = hdop;
                        }
                    }
                    else if (tag == QStringLiteral("time"))
                    {
                        QString text = GpxDocument::readText(parser, QStringLiteral("time"));
                        const auto timestamp = QDateTime::fromString(text, Qt::DateFormat::ISODate);
                        if (timestamp.isValid() && !timestamp.isNull())
                            wptPt->timestamp = timestamp;
                    }
                }
            }
        }
        else if (tok == QXmlStreamReader::TokenType::EndElement)
        {
            std::shared_ptr<GpxExtensions> parse = parserState.top();
            QString tag = parser.name().toString();

            if (tag.toLower() == QStringLiteral("routepointextension"))
                routePointExtension = false;
            if (parse != nullptr && tag == QStringLiteral("extensions"))
            {
                extensionReadMode = false;
                continue;
            }
            if (extensionReadMode && tag == QStringLiteral("route"))
            {
                routeExtension = false;
                continue;
            }
            if (extensionReadMode && tag == QStringLiteral("types"))
            {
                typesExtension = false;
                continue;
            }

            if (tag == QStringLiteral("metadata"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                assert(std::dynamic_pointer_cast<Metadata>(pop) != nullptr);
            }
            else if (tag == QStringLiteral("author"))
            {
                if (auto author = std::dynamic_pointer_cast<Author>(parse))
                    parserState.pop();
            }
            else if (tag == QStringLiteral("copyright"))
            {
                if (auto copyright = std::dynamic_pointer_cast<Copyright>(parse))
                    parserState.pop();
            }
            else if (tag == QStringLiteral("bounds"))
            {
                if (auto bounds = std::dynamic_pointer_cast<Bounds>(parse))
                    parserState.pop();
            }
            else if (tag == QStringLiteral("trkpt"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                assert(std::dynamic_pointer_cast<WptPt>(pop) != nullptr);
            }
            else if (tag == QStringLiteral("wpt"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                assert(std::dynamic_pointer_cast<WptPt>(pop) != nullptr);
            }
            else if (tag == QStringLiteral("rtept"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                assert(std::dynamic_pointer_cast<WptPt>(pop) != nullptr);
            }
            else if (tag == QStringLiteral("trk"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                assert(std::dynamic_pointer_cast<Track>(pop) != nullptr);
            }
            else if (tag == QStringLiteral("rte"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                assert(std::dynamic_pointer_cast<Route>(pop) != nullptr);
            }
            else if (tag == QStringLiteral("trkseg"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                if (auto segment = std::dynamic_pointer_cast<TrkSegment>(pop))
                {
                    segment->routeSegments = routeSegments;
                    segment->routeTypes = routeTypes;
                    routeSegments.clear();
                    routeTypes.clear();
                    if (firstSegment == nullptr)
                        firstSegment = segment;
                }
                assert(std::dynamic_pointer_cast<TrkSegment>(pop) != nullptr);
            }
            else if (tag == QStringLiteral("rpt"))
            {
                std::shared_ptr<GpxExtensions> pop = parserState.pop();
                assert(std::dynamic_pointer_cast<WptPt>(pop) != nullptr);
            }
        }
    }
    if (!routeTrackSegment->points.isEmpty())
        document->tracks.append(routeTrack);
    if (!routeSegments.isEmpty() && !routeTypes.isEmpty() && firstSegment != nullptr)
    {
        firstSegment->routeSegments = routeSegments;
        firstSegment->routeTypes = routeTypes;
    }

    if (parser.hasError())
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML error: %s (%" PRIi64 ", %" PRIi64 ")",
            qPrintable(parser.errorString()),
            parser.lineNumber(),
            parser.columnNumber());
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

OsmAnd::GpxDocument::Link::Link()
{
}

OsmAnd::GpxDocument::Link::~Link()
{
}

OsmAnd::GpxDocument::Author::Author()
{
}

OsmAnd::GpxDocument::Author::~Author()
{
}

OsmAnd::GpxDocument::Copyright::Copyright()
{
}

OsmAnd::GpxDocument::Copyright::~Copyright()
{
}

OsmAnd::GpxDocument::Bounds::Bounds()
        : minlat(std::numeric_limits<double>::quiet_NaN())
        , minlon(std::numeric_limits<double>::quiet_NaN())
        , maxlat(std::numeric_limits<double>::quiet_NaN())
        , maxlon(std::numeric_limits<double>::quiet_NaN())
{
}

OsmAnd::GpxDocument::Bounds::~Bounds()
{
}

OsmAnd::GpxDocument::Metadata::Metadata()
{
}

OsmAnd::GpxDocument::Metadata::~Metadata()
{
}

OsmAnd::GpxDocument::WptPt::WptPt()
        : position(LatLon(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN()))
        , elevation(std::numeric_limits<double>::quiet_NaN())
        , horizontalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
        , verticalDilutionOfPrecision(std::numeric_limits<double>::quiet_NaN())
        , heading(std::numeric_limits<double>::quiet_NaN())
{
}

OsmAnd::GpxDocument::WptPt::~WptPt()
{
}

OsmAnd::GpxDocument::RouteSegment::RouteSegment()
{
}

OsmAnd::GpxDocument::RouteSegment::~RouteSegment()
{
}

OsmAnd::GpxDocument::RouteType::RouteType()
{
}

OsmAnd::GpxDocument::RouteType::~RouteType()
{
}

OsmAnd::GpxDocument::TrkSegment::TrkSegment()
{
}

OsmAnd::GpxDocument::TrkSegment::~TrkSegment()
{
}

OsmAnd::GpxDocument::Track::Track()
{
}

OsmAnd::GpxDocument::Track::~Track()
{
}

OsmAnd::GpxDocument::Route::Route()
{
}

OsmAnd::GpxDocument::Route::~Route()
{
}
