#include "BinaryMapObjectSymbolsGroup.h"

#include "BinaryMapObject.h"

OsmAnd::BinaryMapObjectSymbolsGroup::BinaryMapObjectSymbolsGroup(
    const std::shared_ptr<const Model::BinaryMapObject>& mapObject_,
    const bool sharableById_)
    : MapSymbolsGroupWithId(mapObject_->id, sharableById_)
    , mapObject(mapObject_)
{
}

OsmAnd::BinaryMapObjectSymbolsGroup::~BinaryMapObjectSymbolsGroup()
{
}

QString OsmAnd::BinaryMapObjectSymbolsGroup::getDebugTitle() const
{
    return QString(QLatin1String("MO %1,%2")).arg(id).arg(static_cast<int64_t>(id) / 2);
}
