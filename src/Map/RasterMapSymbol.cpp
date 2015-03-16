#include "RasterMapSymbol.h"

OsmAnd::RasterMapSymbol::RasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : MapSymbol(group_)
    , minDistance(-1.0f)
{
}

OsmAnd::RasterMapSymbol::~RasterMapSymbol()
{
}
