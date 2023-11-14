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

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::FavoriteLocationsGpxCollection_P::loadFrom(const QString& fileName, bool append /*= false*/)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return nullptr;

    QXmlStreamReader reader(&file);
    const auto gpxDocument = loadFrom(reader, append);
    file.close();

    return gpxDocument;
}

std::shared_ptr<OsmAnd::GpxDocument> OsmAnd::FavoriteLocationsGpxCollection_P::loadFrom(QXmlStreamReader& xmlReader, bool append /*= false*/)
{
    const auto gpxDocument = OsmAnd::GpxDocument::loadFrom(xmlReader);
    if (gpxDocument != nullptr)
    {
        QList< std::shared_ptr< FavoriteLocation > > newItems;
        for (const auto& wpt : gpxDocument->points)
        {
            std::shared_ptr< FavoriteLocation > newItem(new FavoriteLocation(wpt->position));
            newItem->setTitle(wpt->name);
            newItem->setGroup(wpt->type);
            newItem->setElevation(QString::number(wpt->elevation));
            newItem->setTime(wpt->timestamp.toString());
            newItem->setDescription(wpt->description);
            newItem->setComment(wpt->comment);
            for (const auto& extension : wpt->extensions)
            {
                if (extension->name == QStringLiteral("address"))
                {
                    newItem->setAddress(extension->value);
                }
                else if (extension->name == QStringLiteral("icon"))
                {
                    newItem->setIcon(extension->value);
                }
                else if (extension->name == QStringLiteral("background"))
                {
                    newItem->setBackground(extension->value);
                }
                else if (extension->name == QStringLiteral("hidden"))
                {
                    newItem->setIsHidden(extension->value == QStringLiteral("true"));
                }
                else if (extension->name == QStringLiteral("calendar_event"))
                {
                    newItem->setCalendarEvent(extension->value == QStringLiteral("true"));
                }
                else if (extension->name == QStringLiteral("creation_date"))
                {
                    newItem->setPickupTime(extension->value);
                }
                else if (extension->name == QStringLiteral("color")
                         || extension->name == QStringLiteral("colour")
                         || extension->name == QStringLiteral("displaycolor")
                         || extension->name == QStringLiteral("displaycolour"))
                {
                    bool ok = false;
                    const auto color = Utilities::parseColor(extension->value, ColorARGB(), &ok);
                    if (ok)
                        newItem->setColor(color);
                }
                else if (extension->name.startsWith(QStringLiteral("amenity_")) || extension->name.startsWith(QStringLiteral("osm_tag_")))
                {
                    newItem->setExtension(extension->name, extension->value);
                    if (extension->name == QStringLiteral("amenity_origin"))
                        newItem->setAmenityOriginName(extension->value);
                }
            }
            newItems.push_back(newItem);
        }
        
        {
            QWriteLocker scopedLocker(&_collectionLock);
            
            if (!append)
                doClearFavoriteLocations();
            
            appendFrom(newItems);
            
            notifyCollectionChanged();
        }
    }
    return gpxDocument;
}
