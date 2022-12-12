#include "FavoriteLocationsGpxCollection_P.h"
#include "FavoriteLocationsGpxCollection.h"

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QFile>
#include <QIODevice>
#include "restore_internal_warnings.h"

#include "ArchiveWriter.h"
#include "FavoriteLocation.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::FavoriteLocationsGpxCollection_P::FavoriteLocationsGpxCollection_P(FavoriteLocationsGpxCollection* const owner_)
    : FavoriteLocationsCollection_P(owner_)
    , owner(owner_)
{
}

OsmAnd::FavoriteLocationsGpxCollection_P::~FavoriteLocationsGpxCollection_P()
{
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::saveTo(const QString& fileName) const
{
    return saveTo(fileName, QString());
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::saveTo(const QString& fileName, const QString& groupName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    const bool ok = saveTo(writer, groupName);
    file.close();

    return ok;
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::loadFrom(const QString& fileName, bool append /*= false*/)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QXmlStreamReader reader(&file);
    const bool ok = loadFrom(reader, append);
    file.close();

    return ok;
}

void OsmAnd::FavoriteLocationsGpxCollection_P::writeExtension(const QString &key, const QString &value, QXmlStreamWriter &writer) const
{
    QString k(key);
    k.replace(QStringLiteral(":"), QStringLiteral("_-_"));
    writer.writeTextElement(QStringLiteral("osmand:") + k, value);
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::saveTo(QXmlStreamWriter& writer) const
{
    return saveTo(writer, QString());
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::saveTo(QXmlStreamWriter& writer, const QString& groupName) const
{
    QReadLocker scopedLocker(&_collectionLock);

    writer.writeStartDocument(QLatin1String("1.0"), true);

    //<gpx
    //      version="1.1"
    //      creator="OsmAnd"
    //      xmlns="http://www.topografix.com/GPX/1/1"
    //      xmlns:osmand="https://osmand.net"
    //      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    //      xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
    writer.writeStartElement(QLatin1String("gpx"));
    writer.writeAttribute(QLatin1String("version"), QLatin1String("1.1"));
    writer.writeAttribute(QLatin1String("creator"), QLatin1String("OsmAnd"));
    writer.writeAttribute(QLatin1String("xmlns"), QLatin1String("http://www.topografix.com/GPX/1/1"));
    writer.writeNamespace(QStringLiteral("https://osmand.net"), QStringLiteral("osmand"));
    writer.writeAttribute(QLatin1String("xmlns:xsi"), QLatin1String("http://www.w3.org/2001/XMLSchema-instance"));
    writer.writeAttribute(QLatin1String("xsi:schemaLocation"), QLatin1String("http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd"));

    for (const auto& item : constOf(_collection))
    {
        const auto group = item->getGroup();
        if (!groupName.isNull() && group != groupName)
            continue;

        // <wpt>
        writer.writeStartElement(QLatin1String("wpt"));
        const auto& latLon = item->getLatLonPrecised();
        writer.writeAttribute(QLatin1String("lat"), QString::number(latLon.latitude, 'f', kFavoriteLatLonPrecision));
        writer.writeAttribute(QLatin1String("lon"), QString::number(latLon.longitude, 'f', kFavoriteLatLonPrecision));

        if (!item->getElevation().isNull() && item->getElevation().toDouble() > 0.)
        {
            // <ele>
            writer.writeTextElement(QLatin1String("ele"), item->getElevation());
        }
        
        if (!item->getComment().isNull())
        {
            // <cmt>
            writer.writeTextElement(QLatin1String("cmt"), item->getComment());
        }
        
        if (!item->getTime().isNull())
        {
            // <time>
            writer.writeTextElement(QLatin1String("time"), item->getTime());
        }
        
        // <name>
        writer.writeTextElement(QLatin1String("name"), item->getTitle());

        // <desc>
        const auto description = item->getDescription();
        if (!description.isEmpty())
            writer.writeTextElement(QLatin1String("desc"), description);

        if (!group.isEmpty())
        {
            // <type>
            writer.writeTextElement(QLatin1String("type"), group);
        }
        
        // <extensions>
        writer.writeStartElement(QLatin1String("extensions"));
        
        // <address>
        const auto address = item->getAddress();
        if (!address.isEmpty())
            writeExtension(QLatin1String("address"), address, writer);
        
        // <icon>
        const auto icon = item->getIcon();
        if (!icon.isEmpty())
            writeExtension(QLatin1String("icon"), icon, writer);
        
        // <background>
        const auto background = item->getBackground();
        if (!background.isEmpty())
            writeExtension(QLatin1String("background"), background, writer);

        const auto color = item->getColor();
        if (color != ColorRGB())
        {
            // <color>
            const auto colorValue = color.toString();
            writeExtension(QLatin1String("color"), colorValue, writer);
        }
        
        // <hidden>
        if (item->isHidden())
            writer.writeEmptyElement(QLatin1String("osmand:hidden"));
        
        // <pickup_date>
        if (!item->getPickupTime().isNull())
            writeExtension(QLatin1String("pickup_date"), item->getPickupTime(), writer);
        
        if (item->getCalendarEvent())
            writeExtension(QLatin1String("calendar_event"), "true", writer);
        
        if (!item->getAmenityOriginName().isNull())
        {
            // <origin_name>
            writeExtension(QLatin1String("amenity_origin"), item->getAmenityOriginName(), writer);
        }
        
        // all other extensions
        for (const auto& extension : rangeOf(item->getExtensions()))
        {
            writeExtension(extension.key(), extension.value(), writer);
        }

        // </extensions>
        writer.writeEndElement();

        // </wpt>
        writer.writeEndElement();
    }

    // </gpx>
    writer.writeEndElement();

    writer.writeEndDocument();

    return true;
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::loadFrom(QXmlStreamReader& xmlReader, bool append /*= false*/)
{
    std::shared_ptr< FavoriteLocation > newItem;
    QList< std::shared_ptr< FavoriteLocation > > newItems;
    bool isInsideMetadataTag = false;
    bool isInsideExtensionsTag = false;

    while (!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (tagName == QLatin1String("metadata"))
                isInsideMetadataTag = true;
            if (isInsideMetadataTag)
                continue;
            
            if (tagName == QLatin1String("extensions"))
            {
                isInsideExtensionsTag = true;
                continue;
            }
            
            if (tagName == QLatin1String("wpt"))
            {
                if (newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <wpt>");
                    return false;
                }

                bool ok = true;
                const double lat = xmlReader.attributes().value(QLatin1String("lat")).toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: invalid latitude");
                    return false;
                }
                const double lon = xmlReader.attributes().value(QLatin1String("lon")).toDouble(&ok);
                if (!ok)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: invalid longitude");
                    return false;
                }

                newItem.reset(new FavoriteLocation(LatLon(lat, lon)));
                newItem->setGroup(QStringLiteral(""));
            }
            else if (tagName == QLatin1String("ele"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <ele>");
                    return false;
                }
       
                newItem->setElevation(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("cmt"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <cmt>");
                    return false;
                }
       
                newItem->setComment(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("amenity_origin"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <amenity_origin>");
                    return false;
                }
       
                newItem->setAmenityOriginName(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("time"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <time>");
                    return false;
                }
       
                newItem->setTime(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("name"))
            {
                // The <metadata> also contains the name tag, so it's ok to have a name tag without the item
                if (newItem)
                {
                    newItem->setTitle(xmlReader.readElementText());
                }
            }
            else if (tagName == QLatin1String("desc"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <desc>");
                    return false;
                }

                newItem->setDescription(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("address"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <address>");
                    return false;
                }
       
                newItem->setAddress(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("calendar_event"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <calendar_event>");
                    return false;
                }
       
                newItem->setCalendarEvent(xmlReader.readElementText() == "true");
            }
            else if (tagName == QLatin1String("pickup_date"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <pickup_date>");
                    return false;
                }
       
                newItem->setPickupTime(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("category") || tagName == QLatin1String("type"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <category>");
                    return false;
                }

                const auto& group = xmlReader.readElementText();
                if (!group.isEmpty())
                    newItem->setGroup(group);
            }
            else if (tagName == QLatin1String("icon"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <icon>");
                    return false;
                }
       
                newItem->setIcon(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("background"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <background>");
                    return false;
                }
       
                newItem->setBackground(xmlReader.readElementText());
            }
            else if (tagName == QLatin1String("color"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <color>");
                    return false;
                }

                bool ok = false;
                const auto color = Utilities::parseColor(xmlReader.readElementText(), ColorARGB(), &ok);
                if (!ok)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: invalid color");
                    continue;
                }

                newItem->setColor(static_cast<ColorRGB>(color));
            }
            else if (tagName == QLatin1String("color"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unexpected <hidden/>");
                    return false;
                }

                newItem->setIsHidden(true);
            }
            // TODO: read group_items from android
            else if (isInsideExtensionsTag && newItem)
            {
                newItem->setExtension(tagName.toString(), xmlReader.readElementText());
            }
        }
        else if (xmlReader.isEndElement())
        {
            if (tagName == QLatin1String("metadata"))
            {
                isInsideMetadataTag = false;
            }
            else if (tagName == QLatin1String("extensions"))
            {
                isInsideExtensionsTag = false;
            }
            else if (tagName == QLatin1String("wpt"))
            {
                if (!newItem)
                {
                    LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired </wpt>");
                    return false;
                }

                newItems.push_back(newItem);
                newItem.reset();
            }
        }
    }
    if (xmlReader.hasError())
    {
        LogPrintf(
            LogSeverityLevel::Warning,
            "XML error: %s (%" PRIi64 ", %" PRIi64 ")",
            qPrintable(xmlReader.errorString()),
            xmlReader.lineNumber(),
            xmlReader.columnNumber());
        return false;
    }

    {
        QWriteLocker scopedLocker(&_collectionLock);

        if (!append)
            doClearFavoriteLocations();

        appendFrom(newItems);

        notifyCollectionChanged();
    }

    return true;
}
