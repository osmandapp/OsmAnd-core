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

QList<OsmAnd::PointI> OsmAnd::BillboardVectorMapSymbol::getAdditionalPositions31() const
{
    return additionalPositions31;
}

void OsmAnd::BillboardVectorMapSymbol::setAdditionalPositions31(const QList<PointI>& additionalPositions31_)
{
    additionalPositions31 = additionalPositions31_;
}
