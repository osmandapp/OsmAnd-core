#include "RasterizerContext_P.h"
#include "RasterizerContext.h"

#include <SkPathEffect.h>

OsmAnd::RasterizerContext_P::RasterizerContext_P( RasterizerContext* owner_ )
    : owner(owner_)
{
}

OsmAnd::RasterizerContext_P::~RasterizerContext_P()
{
    clear();
}

void OsmAnd::RasterizerContext_P::clear()
{
    _primitivesGroups.clear();
    _polygons.clear();
    _polylines.clear();
    _points.clear();

    _symbols.clear();
}
