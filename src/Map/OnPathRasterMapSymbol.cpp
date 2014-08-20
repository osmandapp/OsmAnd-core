#include "OnPathRasterMapSymbol.h"

OsmAnd::OnPathRasterMapSymbol::OnPathRasterMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_)
    : RasterMapSymbol(group_, isShareable_)
{
}

OsmAnd::OnPathRasterMapSymbol::~OnPathRasterMapSymbol()
{
}

QVector<OsmAnd::PointI> OsmAnd::OnPathRasterMapSymbol::getPath31() const
{
    return path31;
}

void OsmAnd::OnPathRasterMapSymbol::setPath31(const QVector<PointI>& newPath31)
{
    path31 = newPath31;
}

OsmAnd::OnPathRasterMapSymbol::PinPoint OsmAnd::OnPathRasterMapSymbol::getPinPointOnPath() const
{
    return pinPointOnPath;
}

void OsmAnd::OnPathRasterMapSymbol::setPinPointOnPath(const PinPoint& pinPoint)
{
    pinPointOnPath = pinPoint;
}
