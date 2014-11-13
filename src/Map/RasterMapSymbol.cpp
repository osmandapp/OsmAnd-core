#include "RasterMapSymbol.h"

OsmAnd::RasterMapSymbol::RasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : MapSymbol(group_)
    , pathPaddingLeft(0.0f)
    , pathPaddingRight(0.0f)
{
}

OsmAnd::RasterMapSymbol::~RasterMapSymbol()
{
}
