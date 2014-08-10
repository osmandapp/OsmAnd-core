#include "MapSymbol.h"

OsmAnd::MapSymbol::MapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_)
    : group(group_)
    , groupPtr(group_.get())
    , isShareable(isShareable_)
    , order(0)
    , contentClass(ContentClass::Unknown)
    , intersectionModeFlags(RegularIntersectionProcessing)
    , isHidden(false)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}
