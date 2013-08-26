#include "RasterizerContext.h"
#include "RasterizerContext_P.h"

OsmAnd::RasterizerContext::RasterizerContext()
    : _d(new RasterizerContext_P(this))
{
}

OsmAnd::RasterizerContext::~RasterizerContext()
{
}
