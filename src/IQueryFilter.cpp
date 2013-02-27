#include "IQueryFilter.h"

#include <limits>
#include "Utilities.h"

OsmAnd::IQueryFilter::IQueryFilter()
    : _zoom(std::numeric_limits<uint32_t>::max())
    , _bboxTop31(Utilities::get31TileNumberY(90.0))
    , _bboxLeft31(Utilities::get31TileNumberX(-180.0))
    , _bboxBottom31(Utilities::get31TileNumberY(-90))
    , _bboxRight31(Utilities::get31TileNumberX(180.0))
{
}

OsmAnd::IQueryFilter::~IQueryFilter()
{
}
