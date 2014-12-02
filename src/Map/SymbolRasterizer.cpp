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
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
    QList< std::shared_ptr<const RasterizedSymbolsGroup> >& outSymbolsGroups,
    const float scaleFactor /*= 1.0f*/,
    const FilterByMapObject filter /*= nullptr*/,
    const IQueryController* const controller /*= nullptr*/)
{
    _p->rasterize(primitivisedObjects, outSymbolsGroups, scaleFactor, filter, controller);
}

OsmAnd::SymbolRasterizer::RasterizedSymbolsGroup::RasterizedSymbolsGroup(const std::shared_ptr<const MapObject>& mapObject_)
    : mapObject(mapObject_)
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
    , pathPaddingLeft(0.0f)
    , pathPaddingRight(0.0f)
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
