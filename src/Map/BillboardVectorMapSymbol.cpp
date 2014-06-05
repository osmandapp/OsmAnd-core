#include "BillboardVectorMapSymbol.h"

OsmAnd::BillboardVectorMapSymbol::BillboardVectorMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_,
    const int order_,
    const IntersectionModeFlags intersectionModeFlags_,
    const PointI& position31_,
    const PointI& offset_)
    : VectorMapSymbol(group_, isShareable_, order_, intersectionModeFlags_)
    , offset(offset_)
    , position31(position31_)
{
}

OsmAnd::BillboardVectorMapSymbol::~BillboardVectorMapSymbol()
{
}

OsmAnd::PointI OsmAnd::BillboardVectorMapSymbol::getOffset() const
{
    return offset;
}

void OsmAnd::BillboardVectorMapSymbol::setOffset(const PointI newOffset)
{
    offset = newOffset;
}

OsmAnd::PointI OsmAnd::BillboardVectorMapSymbol::getPosition31() const
{
    return position31;
}

void OsmAnd::BillboardVectorMapSymbol::setPosition31(const PointI position)
{
    position31 = position;
}
