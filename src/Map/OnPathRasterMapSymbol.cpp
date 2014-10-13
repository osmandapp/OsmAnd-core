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
    return *shareablePath31;
}

std::shared_ptr< const QVector<OsmAnd::PointI> > OsmAnd::OnPathRasterMapSymbol::getPath31SharedRef() const
{
    return shareablePath31;
}

void OsmAnd::OnPathRasterMapSymbol::setPath31(const QVector<PointI>& newPath31)
{
    shareablePath31.reset(new QVector<PointI>(newPath31));
}

void OsmAnd::OnPathRasterMapSymbol::setPath31(const std::shared_ptr< const QVector<PointI> >& newSharedPath31)
{
    shareablePath31 = newSharedPath31;
}

OsmAnd::OnPathRasterMapSymbol::PinPoint OsmAnd::OnPathRasterMapSymbol::getPinPointOnPath() const
{
    return pinPointOnPath;
}

void OsmAnd::OnPathRasterMapSymbol::setPinPointOnPath(const PinPoint& pinPoint)
{
    pinPointOnPath = pinPoint;
}
