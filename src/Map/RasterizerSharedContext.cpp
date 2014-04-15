#include "RasterizerSharedContext.h"
#include "RasterizerSharedContext_P.h"

OsmAnd::RasterizerSharedContext::RasterizerSharedContext()
    : _p(new RasterizerSharedContext_P(this))
{
}

OsmAnd::RasterizerSharedContext::~RasterizerSharedContext()
{
}
