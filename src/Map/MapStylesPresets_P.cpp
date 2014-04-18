#include "MapStylesPresets_P.h"
#include "MapStylesPresets.h"

OsmAnd::MapStylesPresets_P::MapStylesPresets_P(MapStylesPresets* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapStylesPresets_P::~MapStylesPresets_P()
{
}

bool OsmAnd::MapStylesPresets_P::loadFrom(const QByteArray& content)
{
    return loadFrom(QXmlStreamReader(content));
}

bool OsmAnd::MapStylesPresets_P::loadFrom(QIODevice& ioDevice)
{
    return loadFrom(QXmlStreamReader(&ioDevice));
}

bool OsmAnd::MapStylesPresets_P::loadFrom(QXmlStreamReader& xmlReader)
{
    QHash< QString, std::shared_ptr<MapStylePreset> > collection;
    std::shared_ptr<MapStylePreset> mapStylePreset;

    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        const auto tagName = xmlReader.name();
        if(xmlReader.isStartElement())
        {
            if(tagName == QLatin1String("mapStylePreset"))
            {
                const auto name = xmlReader.attributes().value(QLatin1String("name")).toString();
                const auto styleName = xmlReader.attributes().value(QLatin1String("styleName")).toString();
                mapStylePreset.reset(new MapStylePreset(name, styleName));
            }
            else if(tagName == QLatin1String("attribute"))
            {
                const auto name = xmlReader.attributes().value(QLatin1String("name")).toString();
                const auto value = xmlReader.attributes().value(QLatin1String("value")).toString();

                mapStylePreset->attributes.insert(name, value);
            }
        }
        else if(xmlReader.isEndElement())
        {
            if(tagName == QLatin1String("mapStylePreset"))
            {
                if(!collection.contains(mapStylePreset->name))
                    collection.insert(mapStylePreset->name, mapStylePreset);
                else
                    LogPrintf(LogSeverityLevel::Warning, "Ignored duplicate map style preset with name '%s'", qPrintable(mapStylePreset->name));
                mapStylePreset.reset();
            }
        }
    }
    if(xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning, "XML error: %s (%d, %d)", qPrintable(xmlReader.errorString()), xmlReader.lineNumber(), xmlReader.columnNumber());
        return false;
    }

    _collection = collection;

    return true;
}

bool OsmAnd::MapStylesPresets_P::saveTo(QIODevice& ioDevice) const
{
    return saveTo(QXmlStreamWriter(&ioDevice));
}

bool OsmAnd::MapStylesPresets_P::saveTo(QXmlStreamWriter& xmlWriter) const
{
    assert(false);
    return false;
}

QList< std::shared_ptr<OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresets_P::getCollection() const
{
    return _collection.values();
}

std::shared_ptr<OsmAnd::MapStylePreset> OsmAnd::MapStylesPresets_P::getPresetByName(const QString& name) const
{
    const auto citPreset = _collection.constFind(name);
    if(citPreset == _collection.cend())
        return nullptr;
    return *citPreset;
}

bool OsmAnd::MapStylesPresets_P::addPreset(const std::shared_ptr<MapStylePreset>& preset)
{
    if(_collection.constFind(preset->name) != _collection.cend())
        return false;

    _collection.insert(preset->name, preset);

    return true;
}

bool OsmAnd::MapStylesPresets_P::removePreset(const QString& name)
{
    return (_collection.remove(name) > 0);
}
