#include "CirclePrimitiveMapSymbol.h"

OsmAnd::CirclePrimitiveMapSymbol::CirclePrimitiveMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_)
    : PrimitiveMapSymbol(group_, isShareable_, order_, intersectionModeFlags_)
{
}

OsmAnd::CirclePrimitiveMapSymbol::~CirclePrimitiveMapSymbol()
{
}

