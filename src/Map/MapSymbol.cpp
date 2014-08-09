#include "MapSymbol.h"

OsmAnd::MapSymbol::MapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_)
    : group(group_)
    , groupPtr(group_.get())
    , isShareable(isShareable_)
    , order(order_)
    , contentClass(ContentClass::Unknown)
    , intersectionModeFlags(intersectionModeFlags_)
    , isHidden(false)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}
