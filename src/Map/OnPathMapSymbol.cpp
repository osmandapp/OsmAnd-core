#include "OnPathMapSymbol.h"

OsmAnd::OnPathMapSymbol::OnPathMapSymbol(
    const std::shared_ptr<MapSymbolsGroup>& group_,
    const bool isShareable_)
    : RasterMapSymbol(group_, isShareable_)
{
}

OsmAnd::OnPathMapSymbol::~OnPathMapSymbol()
{
}

OsmAnd::OnPathMapSymbol::PinPoint::PinPoint()
    : basePathPointIndex(0)
    , offsetFromBasePathPoint(0.0)
    , kOffsetFromBasePathPoint(0.0f)
{
}

OsmAnd::OnPathMapSymbol::PinPoint::PinPoint(
    const unsigned int basePathPointIndex_,
    const double offsetFromBasePathPoint_,
    const float kOffsetFromBasePathPoint_,
    const PointI& point_)
    : basePathPointIndex(basePathPointIndex_)
    , offsetFromBasePathPoint(offsetFromBasePathPoint_)
    , kOffsetFromBasePathPoint(kOffsetFromBasePathPoint_)
    , point(point_)
{
}

OsmAnd::OnPathMapSymbol::PinPoint::~PinPoint()
{
}
