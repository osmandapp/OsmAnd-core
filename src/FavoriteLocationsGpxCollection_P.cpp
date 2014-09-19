#include "FavoriteLocationsGpxCollection_P.h"
#include "FavoriteLocationsGpxCollection.h"

#include <QFile>
#include <QIODevice>

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

bool OsmAnd::FavoriteLocationsGpxCollection_P::saveTo(const QString& filename) const
{
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
		return false;

	QXmlStreamWriter writer(&file);
	writer.setAutoFormatting(true);
	const bool ok = saveTo(writer);
	file.close();

	return ok;
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::loadFrom(const QString& filename)
{
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QXmlStreamReader reader(&file);
	const bool ok = loadFrom(reader);
	file.close();

	return ok;
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::saveTo(QXmlStreamWriter& writer) const
{
	QReadLocker scopedLocker(&_collectionLock);

	writer.writeStartDocument(QLatin1String("1.0"), true);
	
	//<gpx
	//	  version="1.1"
	//	  creator="OsmAnd"
	//	  xmlns="http://www.topografix.com/GPX/1/1"
	//	  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
	//	  xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd">
	writer.writeStartElement(QLatin1String("gpx"));
	writer.writeAttribute(QLatin1String("version"), QLatin1String("1.1"));
	writer.writeAttribute(QLatin1String("creator"), QLatin1String("OsmAnd"));
	writer.writeAttribute(QLatin1String("xmlns"), QLatin1String("http://www.topografix.com/GPX/1/1"));
	writer.writeAttribute(QLatin1String("xmlns:xsi"), QLatin1String("http://www.w3.org/2001/XMLSchema-instance"));
	writer.writeAttribute(QLatin1String("xsi:schemaLocation"), QLatin1String("http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd"));

	for (const auto& item : constOf(_collection))
	{
		// <wpt>
		writer.writeStartElement(QLatin1String("wpt"));
		writer.writeAttribute(QLatin1String("lat"), QString::number(item->latLon.latitude, 'f', 12));
        writer.writeAttribute(QLatin1String("lon"), QString::number(item->latLon.longitude, 'f', 12));

		// <name>
		writer.writeTextElement(QLatin1String("name"), item->getTitle());

		const auto group = item->getGroup();
		if (!group.isEmpty())
		{
			// <category>
			writer.writeTextElement(QLatin1String("category"), group);
		}

		const auto color = item->getColor();
		if (color != ColorRGB())
		{
			// <extensions>
			writer.writeStartElement(QLatin1String("extensions"));

			// <color>
            const auto colorValue = QLatin1Char('#') + QString::number(static_cast<ColorARGB>(color).argb, 16).right(6);
			writer.writeTextElement(QLatin1String("color"), colorValue);

            // <hidden>
            if (item->isHidden())
                writer.writeEmptyElement(QLatin1String("hidden"));

			// </extensions>
			writer.writeEndElement();
		}

		// </wpt>
		writer.writeEndElement();
	}

	// </gpx>
	writer.writeEndElement();
	
	writer.writeEndDocument();

	return true;
}

bool OsmAnd::FavoriteLocationsGpxCollection_P::loadFrom(QXmlStreamReader& reader)
{
	std::shared_ptr< FavoriteLocation > newItem;
	QList< std::shared_ptr< FavoriteLocation > > newItems;

	while (!reader.atEnd() && !reader.hasError())
	{
		reader.readNext();
		const auto tagName = reader.name();
		if (reader.isStartElement())
		{
			if (tagName == QLatin1String("wpt"))
			{
				if (newItem)
				{
					LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <wpt>");
					return false;
				}

				bool ok = true;
				const double lat = reader.attributes().value(QLatin1String("lat")).toDouble(&ok);
				if (!ok)
				{
					LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: invalid latitude");
					return false;
				}
				const double lon = reader.attributes().value(QLatin1String("lon")).toDouble(&ok);
				if (!ok)
				{
					LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: invalid longitude");
					return false;
				}

				newItem.reset(new FavoriteLocation(LatLon(lat, lon)));
			}
			else if (tagName == QLatin1String("name"))
			{
				if (!newItem)
				{
					LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <name>");
					return false;
				}

				newItem->setTitle(reader.readElementText());
			}
			else if (tagName == QLatin1String("category"))
			{
				if (!newItem)
				{
					LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <category>");
					return false;
				}

                const auto& group = reader.readElementText();
                if (!group.isEmpty())
                    newItem->setGroup(group);
			}
			else if (tagName == QLatin1String("color"))
			{
				if (!newItem)
				{
					LogPrintf(LogSeverityLevel::Warning, "Malformed favorites GPX file: unpaired <color>");
					return false;
				}

				bool ok = false;
				const auto color = Utilities::parseColor(reader.readElementText(), ColorARGB(), &ok);
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
		}
		else if (reader.isEndElement())
		{
			if (tagName == QLatin1String("wpt"))
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
	if (reader.hasError())
	{
		LogPrintf(LogSeverityLevel::Warning, "XML error: %s (%d, %d)", qPrintable(reader.errorString()), reader.lineNumber(), reader.columnNumber());
		return false;
	}

	{
		QWriteLocker scopedLocker(&_collectionLock);

		doClearFavoriteLocations();
		appendFrom(newItems);

		notifyCollectionChanged();
	}

	return true;
}
