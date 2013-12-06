#include "IMapSymbolProvider.h"

#include <SkBitmap.h>

OsmAnd::IMapSymbolProvider::IMapSymbolProvider()
{
}

OsmAnd::IMapSymbolProvider::~IMapSymbolProvider()
{
}

bool OsmAnd::IMapSymbolProvider::canSymbolsBeSharedFrom(const std::shared_ptr<const Model::MapObject>& mapObject)
{
    return false;
}

OsmAnd::MapSymbolsGroup::MapSymbolsGroup(const std::shared_ptr<const Model::MapObject>& mapObject_)
    : mapObject(mapObject_)
{
}

OsmAnd::MapSymbolsGroup::~MapSymbolsGroup()
{
}

OsmAnd::MapSymbol::MapSymbol(const std::weak_ptr<const MapSymbolsGroup>& group_, const std::shared_ptr<const Model::MapObject>& mapObject_, const int order_, const PointI& location_, const std::shared_ptr<const SkBitmap>& bitmap_)
    : _bitmap(bitmap_)
    , group(group_)
    , mapObject(mapObject_)
    , order(order_)
    , location(location_)
    , bitmap(_bitmap)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}

void OsmAnd::MapSymbol::releaseNonRetainedData()
{
    _bitmap.reset();
}

OsmAnd::MapSymbolsTile::MapSymbolsTile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_)
    : _symbolsGroups(symbolsGroups_)
    , symbolsGroups(_symbolsGroups)
{
}

OsmAnd::MapSymbolsTile::~MapSymbolsTile()
{
}
