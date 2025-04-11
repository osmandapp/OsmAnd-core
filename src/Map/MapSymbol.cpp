#include "MapSymbol.h"

#include "MapSymbolsGroup.h"

OsmAnd::MapSymbol::MapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_)
    : group(group_)
    , groupPtr(group_.get())
    , groupHasSharingKey([group_]() -> bool { MapSymbolsGroup::SharingKey sharingKey; return group_->obtainSharingKey(sharingKey); }())
    , order(0)
    , contentClass(ContentClass::Unknown)
    , isHidden(false)
    , ignoreClick(false)
    , subsection(0)
    , allowFastCheckByFrustum(true)
{
}

OsmAnd::MapSymbol::~MapSymbol()
{
}
