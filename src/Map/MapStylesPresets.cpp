#include "MapStylesPresets.h"
#include "MapStylesPresets_P.h"

#include <QFile>

OsmAnd::MapStylesPresets::MapStylesPresets()
    : _p(new MapStylesPresets_P(this))
{
}

OsmAnd::MapStylesPresets::~MapStylesPresets()
{
}

bool OsmAnd::MapStylesPresets::loadFrom(const QByteArray& content)
{
    return _p->loadFrom(content);
}

bool OsmAnd::MapStylesPresets::loadFrom(QIODevice& ioDevice)
{
    return _p->loadFrom(ioDevice);
}

bool OsmAnd::MapStylesPresets::loadFrom(const QString& fileName)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    const auto success = loadFrom(file);
    file.close();
    return success;
}

bool OsmAnd::MapStylesPresets::saveTo(QIODevice& ioDevice) const
{
    return _p->saveTo(ioDevice);
}

bool OsmAnd::MapStylesPresets::saveTo(const QString& fileName) const
{
    QFile file(fileName);
    if(!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;
    const auto success = saveTo(file);
    if(!success)
    {
        file.close();
        file.remove();
    }
    return success;
}

QList< std::shared_ptr<OsmAnd::MapStylePreset> > OsmAnd::MapStylesPresets::getCollection() const
{
    return _p->getCollection();
}

std::shared_ptr<OsmAnd::MapStylePreset> OsmAnd::MapStylesPresets::getPresetByName(const QString& name) const
{
    return _p->getPresetByName(name);
}

bool OsmAnd::MapStylesPresets::addPreset(const std::shared_ptr<MapStylePreset>& preset)
{
    return _p->addPreset(preset);
}

bool OsmAnd::MapStylesPresets::removePreset(const QString& name)
{
    return _p->removePreset(name);
}
