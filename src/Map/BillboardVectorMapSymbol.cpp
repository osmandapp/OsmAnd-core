#include "BillboardVectorMapSymbol.h"

OsmAnd::BillboardVectorMapSymbol::BillboardVectorMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_)
    : VectorMapSymbol(group_, isShareable_)
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
