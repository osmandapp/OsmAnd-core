#include "IOnSurfaceMapSymbol.h"

OsmAnd::IOnSurfaceMapSymbol::IOnSurfaceMapSymbol()
{

}

OsmAnd::IOnSurfaceMapSymbol::~IOnSurfaceMapSymbol()
{

}

bool OsmAnd::IOnSurfaceMapSymbol::isAzimuthAlignedDirection() const
{
    return qIsNaN(getDirection());
}
