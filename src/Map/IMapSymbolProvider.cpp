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

OsmAnd::MapSymbol::MapSymbol(
    const std::weak_ptr<const MapSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_)
    : _bitmap(bitmap_)
    , group(group_)
    , mapObject(mapObject_)
    , bitmap(_bitmap)
    , order(order_)
    , content(content_)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}

void OsmAnd::MapSymbol::releaseNonRetainedData()
{
    _bitmap.reset();
}

OsmAnd::MapPinnedSymbol::MapPinnedSymbol(
    const std::weak_ptr<const MapSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const PointI& location31_,
    const PointI& offset_)
    : MapSymbol(group_, mapObject_, bitmap_, order_, content_)
    , location31(location31_)
    , offset(offset_)
{
}

OsmAnd::MapPinnedSymbol::~MapPinnedSymbol()
{
}

OsmAnd::MapSymbol* OsmAnd::MapPinnedSymbol::cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& replacementBitmap) const
{
    return new MapPinnedSymbol(group, mapObject, replacementBitmap, order, content, location31, offset);
}

OsmAnd::MapSymbolOnPath::MapSymbolOnPath(
    const std::weak_ptr<const MapSymbolsGroup>& group_,
    const std::shared_ptr<const Model::MapObject>& mapObject_,
    const std::shared_ptr<const SkBitmap>& bitmap_,
    const int order_,
    const QString& content_,
    const QVector<float>& glyphsWidth_)
    : MapSymbol(group_, mapObject_, bitmap_, order_, content_)
    , glyphsWidth(glyphsWidth_)
{
}

OsmAnd::MapSymbolOnPath::~MapSymbolOnPath()
{
}

OsmAnd::MapSymbol* OsmAnd::MapSymbolOnPath::cloneWithReplacedBitmap(const std::shared_ptr<const SkBitmap>& replacementBitmap) const
{
    return new MapSymbolOnPath(group, mapObject, replacementBitmap, order, content, glyphsWidth);
}

OsmAnd::MapSymbolsTile::MapSymbolsTile(const QList< std::shared_ptr<const MapSymbolsGroup> >& symbolsGroups_)
    : _symbolsGroups(symbolsGroups_)
    , symbolsGroups(_symbolsGroups)
{
}

OsmAnd::MapSymbolsTile::~MapSymbolsTile()
{
}
