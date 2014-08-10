#include "RasterMapSymbol.h"

OsmAnd::RasterMapSymbol::RasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_)
    : MapSymbol(group_, isShareable_)
{
}

OsmAnd::RasterMapSymbol::~RasterMapSymbol()
{
}
