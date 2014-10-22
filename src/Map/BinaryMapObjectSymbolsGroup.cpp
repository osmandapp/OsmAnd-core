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
    return id.id;
}

bool OsmAnd::BinaryMapObjectSymbolsGroup::isSharableById() const
{
    return sharableById;
}

QString OsmAnd::BinaryMapObjectSymbolsGroup::getDebugTitle() const
{
    return id.toString();
}
