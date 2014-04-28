#include "MapStylesPresetsCollection.h"
#include "MapStylesPresetsCollection_P.h"

#include <QFile>

OsmAnd::MapStylesPresetsCollection::MapStylesPresetsCollection()
    : _p(new MapStylesPresetsCollection_P(this))
{
}

OsmAnd::MapStylesPresetsCollection::~MapStylesPresetsCollection()
{
}

bool OsmAnd::MapStylesPresetsCollection::loadFrom(const QByteArray& content)
{
    return _p->loadFrom(content);
}

bool OsmAnd::MapStylesPresetsCollection::loadFrom(QIODevice& ioDevice)
{
    return _p->loadFrom(ioDevice);
}

bool OsmAnd::MapStylesPresetsCollection::loadFrom(const QString& fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const auto success = loadFrom(file);
    file.close();
    return success;
}

bool OsmAnd::MapStylesPresetsCollection::saveTo(QIODevice& ioDevice) const
{
    return _p->saveTo(ioDevice);
}

bool OsmAnd::MapStylesPresetsCollection::saveTo(const QString& fileName) const
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    const auto success = saveTo(file);
    if (!success)
    {
        file.close();
        file.remove();
    }
    return success;
}

bool OsmAnd::MapStylesPresetsCollection::addPreset(const std::shared_ptr<MapStylePreset>& preset)
{
    return _p->addPreset(preset);
}

bool OsmAnd::MapStylesPresetsCollection::removePreset(const std::shared_ptr<MapStylePreset>& preset)
{
    return _p->removePreset(preset);
}

QList< std::shared_ptr<const OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection::getCollection() const
{
    return _p->getCollection();
}

QList< std::shared_ptr<OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection::getCollection()
{
    return _p->getCollection();
}

QList< std::shared_ptr<const OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection::getCollectionFor(const QString& styleName) const
{
    return _p->getCollectionFor(styleName);
}

QList< std::shared_ptr<OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresetsCollection::getCollectionFor(const QString& styleName)
{
    return _p->getCollectionFor(styleName);
}

std::shared_ptr<const OsmAnd::MapStylePreset> OsmAnd::MapStylesPresetsCollection::getPreset(const QString& styleName, const QString& presetName) const
{
    return _p->getPreset(styleName, presetName);
}

std::shared_ptr<OsmAnd::MapStylePreset> OsmAnd::MapStylesPresetsCollection::getPreset(const QString& styleName, const QString& presetName)
{
    return _p->getPreset(styleName, presetName);
}
