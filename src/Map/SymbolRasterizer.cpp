#include "SymbolRasterizer.h"
#include "SymbolRasterizer_P.h"

OsmAnd::SymbolRasterizer::SymbolRasterizer(
    const std::shared_ptr<const TextRasterizer>& textRasterizer_ /*= TextRasterizer::getDefault()*/)
    : _p(new SymbolRasterizer_P(this))
    , textRasterizer(textRasterizer_)
{
}

OsmAnd::SymbolRasterizer::~SymbolRasterizer()
{
}

void OsmAnd::SymbolRasterizer::rasterize(
    const MapPrimitiviser::SymbolsGroupsCollection& symbolsGroups,
    const std::shared_ptr<const MapPresentationEnvironment>& mapPresentationEnvironment,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    const FilterBySymbolsGroup filter /*= nullptr*/,
    const std::shared_ptr<const IQueryController>& queryController /*= nullptr*/) const
{
    _p->rasterize(symbolsGroups, mapPresentationEnvironment, outSymbolsGroups, filter, queryController);
}

OsmAnd::SymbolRasterizer::RasterizedSymbolsGroup::RasterizedSymbolsGroup(
    const std::shared_ptr<const MapObject>& mapObject_, bool canBeShownWithoutIcon_ /* = false */)
    : mapObject(mapObject_)
    , canBeShownWithoutIcon(canBeShownWithoutIcon_)
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbolsGroup::~RasterizedSymbolsGroup()
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbol::RasterizedSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol_)
    : group(group_)
    , primitiveSymbol(primitiveSymbol_)
    , minDistance(-1.0f)
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbol::~RasterizedSymbol()
{
}

OsmAnd::SymbolRasterizer::RasterizedSpriteSymbol::RasterizedSpriteSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol_)
    : RasterizedSymbol(group_, primitiveSymbol_)
{
}

OsmAnd::SymbolRasterizer::RasterizedSpriteSymbol::~RasterizedSpriteSymbol()
{
}

OsmAnd::SymbolRasterizer::RasterizedOnPathSymbol::RasterizedOnPathSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const MapPrimitiviser::Symbol>& primitiveSymbol_)
    : RasterizedSymbol(group_, primitiveSymbol_)
{
}

OsmAnd::SymbolRasterizer::RasterizedOnPathSymbol::~RasterizedOnPathSymbol()
{
}
