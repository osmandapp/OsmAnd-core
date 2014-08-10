#include "BillboardRasterMapSymbol.h"

OsmAnd::BillboardRasterMapSymbol::BillboardRasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_)
    : RasterMapSymbol(group_, isShareable_)
{
}

OsmAnd::BillboardRasterMapSymbol::~BillboardRasterMapSymbol()
{
}

OsmAnd::PointI OsmAnd::BillboardRasterMapSymbol::getOffset() const
{
    return offset;
}

void OsmAnd::BillboardRasterMapSymbol::setOffset(const PointI newOffset)
{
    offset = newOffset;
}

OsmAnd::PointI OsmAnd::BillboardRasterMapSymbol::getPosition31() const
{
    return position31;
}

void OsmAnd::BillboardRasterMapSymbol::setPosition31(const PointI position)
{
    position31 = position;
}
