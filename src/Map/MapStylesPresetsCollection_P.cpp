#include "MapStylesPresetsCollection_P.h"
#include "MapStylesPresetsCollection.h"

#include "MapStylePreset.h"
#include "Logging.h"

OsmAnd::MapStylesPresetsCollection_P::MapStylesPresetsCollection_P(MapStylesPresetsCollection* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapStylesPresetsCollection_P::~MapStylesPresetsCollection_P()
{
}

bool OsmAnd::MapStylesPresetsCollection_P::loadFrom(const QByteArray& content)
{
    QXmlStreamReader xmlReader(content);
    return deserializeFrom(xmlReader);
}

bool OsmAnd::MapStylesPresetsCollection_P::loadFrom(QIODevice& ioDevice)
{
    QXmlStreamReader xmlReader(&ioDevice);
    return deserializeFrom(xmlReader);
}

bool OsmAnd::MapStylesPresetsCollection_P::deserializeFrom(QXmlStreamReader& xmlReader)
{
    QList< std::shared_ptr<MapStylePreset> > order;
    QHash< QString, QHash< QString, std::shared_ptr<MapStylePreset> > > collection;
    std::shared_ptr<MapStylePreset> mapStylePreset;

    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if (xmlReader.isStartElement())
        {
            if (tagName == QLatin1String("mapStylePreset"))
            {
                const auto type_ = xmlReader.attributes().value(QLatin1String("type")).toString();
                auto type = MapStylePreset::Type::Custom;
                if (!type_.isEmpty())
                {
                    if (type_ == QLatin1String("general"))
                        type = MapStylePreset::Type::General;
                    else if (type_ == QLatin1String("pedestrian"))
                        type = MapStylePreset::Type::Pedestrian;
                    else if (type_ == QLatin1String("bicycle"))
                        type = MapStylePreset::Type::Bicycle;
                    else if (type_ == QLatin1String("car"))
                        type = MapStylePreset::Type::Car;
                    else if (type_ != QLatin1String("custom"))
                    {
                        LogPrintf(LogSeverityLevel::Warning, "Ignored map style preset with unknown type '%s'", qPrintable(type_));
                        continue;
                    }
                }

                auto name = xmlReader.attributes().value(QLatin1String("name")).toString();
                if (type != MapStylePreset::Type::Custom && name.isEmpty())
                    name = QLatin1String("type_") + type_;

                auto styleName = xmlReader.attributes().value(QLatin1String("style")).toString();
                if (type == MapStylePreset::Type::Custom && name.isEmpty())
                {
                    LogPrintf(LogSeverityLevel::Warning, "Ignored map style preset with empty name and custom type");
                    continue;
                }
                if (styleName.isEmpty())
                {
                    LogPrintf(LogSeverityLevel::Warning, "Ignored orphaned map style preset with name '%s' and type '%s'", qPrintable(name), qPrintable(type_));
                    continue;
                }
                mapStylePreset.reset(new MapStylePreset(type, name, styleName));
            }
            else if (tagName == QLatin1String("attribute"))
            {
                if (!mapStylePreset)
                    continue;

                const auto name = xmlReader.attributes().value(QLatin1String("name")).toString();
                const auto value = xmlReader.attributes().value(QLatin1String("value")).toString();

                mapStylePreset->attributes.insert(name, value);
            }
        }
        else if (xmlReader.isEndElement())
        {
            if (tagName == QLatin1String("mapStylePreset"))
            {
                auto& collectionForStyle = collection[mapStylePreset->styleName];
                
                if (!collectionForStyle.contains(mapStylePreset->name))
                {
                    collectionForStyle.insert(mapStylePreset->name, mapStylePreset);
                    order.push_back(mapStylePreset);
                }
                else
                    LogPrintf(LogSeverityLevel::Warning, "Ignored duplicate map style preset with name '%s'", qPrintable(mapStylePreset->name));
                mapStylePreset.reset();
            }
        }
    }
    if (xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning, "XML error: %s (%d, %d)", qPrintable(xmlReader.errorString()), xmlReader.lineNumber(), xmlReader.columnNumber());
        return false;
    }

    _order = order;
    _collection = collection;

    return true;
}

bool OsmAnd::MapStylesPresetsCollection_P::saveTo(QIODevice& ioDevice) const
{
    QXmlStreamWriter xmlWriter(&ioDevice);
    return serializeTo(xmlWriter);
}

bool OsmAnd::MapStylesPresetsCollection_P::serializeTo(QXmlStreamWriter& xmlWriter) const
{
    assert(false);
    return false;
}

bool OsmAnd::MapStylesPresetsCollection_P::addPreset(const std::shared_ptr<MapStylePreset>& preset)
{
    auto& collectionForStyle = _collection[preset->styleName];
    if (collectionForStyle.constFind(preset->name) != collectionForStyle.cend())
        return false;

    collectionForStyle.insert(preset->name, preset);
    _order.push_back(preset);

    return true;
}

bool OsmAnd::MapStylesPresetsCollection_P::removePreset(const std::shared_ptr<MapStylePreset>& preset)
{
    const auto itCollectionForStyle = _collection.find(preset->styleName);
    if (itCollectionForStyle == _collection.end())
        return false;
    auto& collectionForStyle = *itCollectionForStyle;
    if (collectionForStyle.remove(preset->name) == 0)
        return false;
    
    _order.removeOne(preset);
    if (collectionForStyle.isEmpty())
        _collection.erase(itCollectionForStyle);

    return true;
}

QList< std::shared_ptr<const OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection_P::getCollection() const
{
    QList< std::shared_ptr<const MapStylePreset> > result;

    for(const auto& preset : constOf(_order))
        result.push_back(preset);

    return result;
}

QList< std::shared_ptr<OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection_P::getCollection()
{
    return _order;
}

QList< std::shared_ptr<const OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection_P::getCollectionFor(const QString& styleName) const
{
    if (!_collection.contains(styleName))
        return QList< std::shared_ptr<const MapStylePreset> >();

    QList< std::shared_ptr<const MapStylePreset> > result;

    for(const auto& preset : constOf(_order))
    {
        if (preset->styleName != styleName)
            continue;
        result.push_back(preset);
    }

    return result;
}

QList< std::shared_ptr<OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection_P::getCollectionFor(const QString& styleName)
{
    if (!_collection.contains(styleName))
        return QList< std::shared_ptr<MapStylePreset> >();

    QList< std::shared_ptr<MapStylePreset> > result;

    for(const auto& preset : constOf(_order))
    {
        if (preset->styleName != styleName)
            continue;
        result.push_back(preset);
    }

    return result;
}

std::shared_ptr<const OsmAnd::MapStylePreset> OsmAnd::MapStylesPresetsCollection_P::getPreset(const QString& styleName, const QString& presetName) const
{
    const auto citCollectionForStyle = _collection.constFind(styleName);
    if (citCollectionForStyle == _collection.cend())
        return nullptr;

    const auto& collectionForStyle = *citCollectionForStyle;
    const auto citPreset = collectionForStyle.constFind(presetName);
    if (citPreset == collectionForStyle.cend())
        return nullptr;

    return *citPreset;
}

std::shared_ptr<OsmAnd::MapStylePreset> OsmAnd::MapStylesPresetsCollection_P::getPreset(const QString& styleName, const QString& presetName)
{
    const auto citCollectionForStyle = _collection.constFind(styleName);
    if (citCollectionForStyle == _collection.cend())
        return nullptr;

    const auto& collectionForStyle = *citCollectionForStyle;
    const auto citPreset = collectionForStyle.constFind(presetName);
    if (citPreset == collectionForStyle.cend())
        return nullptr;

    return *citPreset;
}
