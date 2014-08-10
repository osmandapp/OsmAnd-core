#include "OnPathMapSymbol.h"

OsmAnd::OnPathMapSymbol::OnPathMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_)
    : RasterMapSymbol(group_, isShareable_)
{
}

OsmAnd::OnPathMapSymbol::~OnPathMapSymbol()
{
}
