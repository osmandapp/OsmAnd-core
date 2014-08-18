#include "BinaryMapObjectSymbolsGroup.h"

#include "BinaryMapObject.h"

OsmAnd::BinaryMapObjectSymbolsGroup::BinaryMapObjectSymbolsGroup(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject_,
    const bool sharableById_)
    : mapObject(mapObject_)
    , id(mapObject_->id)
    , sharableById(sharableById_)
{
}

OsmAnd::BinaryMapObjectSymbolsGroup::~BinaryMapObjectSymbolsGroup()
{
}

uint64_t OsmAnd::BinaryMapObjectSymbolsGroup::getId() const
{
    return id;
}

bool OsmAnd::BinaryMapObjectSymbolsGroup::isSharableById() const
{
    return sharableById;
}

QString OsmAnd::BinaryMapObjectSymbolsGroup::getDebugTitle() const
{
    if (static_cast<int64_t>(id) >= 0)
        return QString(QLatin1String("OSM #%1")).arg(id >> 1);
    return QString(QLatin1String("MO #%1")).arg(static_cast<int64_t>(id));
}
