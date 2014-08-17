#include "SymbolRasterizer.h"
#include "SymbolRasterizer_P.h"

OsmAnd::SymbolRasterizer::SymbolRasterizer()
    : _p(new SymbolRasterizer_P(this))
{
}

OsmAnd::SymbolRasterizer::~SymbolRasterizer()
{
}

void OsmAnd::SymbolRasterizer::rasterize(
    const std::shared_ptr<const Primitiviser::PrimitivisedArea>& primitivizedArea,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    std::function<bool (const std::shared_ptr<const Model::BinaryMapObject>& mapObject)> filter /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/)
{
    _p->rasterize(primitivizedArea, outSymbolsGroups, filter, controller);
}

OsmAnd::SymbolRasterizer::RasterizedSymbolsGroup::RasterizedSymbolsGroup(const std::shared_ptr<const Model::BinaryMapObject>& mapObject_)
    : mapObject(mapObject_)
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbolsGroup::~RasterizedSymbolsGroup()
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbol::RasterizedSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Primitiviser::Symbol>& primitiveSymbol_)
    : group(group_)
    , primitiveSymbol(primitiveSymbol_)
{
}

OsmAnd::SymbolRasterizer::RasterizedSymbol::~RasterizedSymbol()
{
}

OsmAnd::SymbolRasterizer::RasterizedSpriteSymbol::RasterizedSpriteSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Primitiviser::Symbol>& primitiveSymbol_)
    : RasterizedSymbol(group_, primitiveSymbol_)
{
}

OsmAnd::SymbolRasterizer::RasterizedSpriteSymbol::~RasterizedSpriteSymbol()
{
}

OsmAnd::SymbolRasterizer::RasterizedOnPathSymbol::RasterizedOnPathSymbol(
    const std::shared_ptr<const RasterizedSymbolsGroup>& group_,
    const std::shared_ptr<const Primitiviser::Symbol>& primitiveSymbol_)
    : RasterizedSymbol(group_, primitiveSymbol_)
{
}

OsmAnd::SymbolRasterizer::RasterizedOnPathSymbol::~RasterizedOnPathSymbol()
{
}
