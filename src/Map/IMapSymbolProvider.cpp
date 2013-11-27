#include "IMapSymbolProvider.h"

#include <SkBitmap.h>

OsmAnd::IMapSymbolProvider::IMapSymbolProvider()
{
}

OsmAnd::IMapSymbolProvider::~IMapSymbolProvider()
{
}

OsmAnd::MapSymbolsGroup::MapSymbolsGroup(const std::shared_ptr<const Model::MapObject>& mapObject_)
    : mapObject(mapObject_)
{
}

OsmAnd::MapSymbolsGroup::~MapSymbolsGroup()
{
}

OsmAnd::MapSymbol::MapSymbol(const std::shared_ptr<const MapSymbolsGroup>& group_, const std::shared_ptr<const Model::MapObject>& mapObject_, const int order_, const PointI& location_, const std::shared_ptr<const SkBitmap>& bitmap_)
    : group(group_)
    , mapObject(mapObject_)
    , order(order_)
    , location(location_)
    , bitmap(bitmap_)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}
