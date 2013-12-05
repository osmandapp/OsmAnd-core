#include "RasterizerContext.h"
#include "RasterizerContext_P.h"

#include "RasterizerSharedContext.h"

OsmAnd::RasterizerContext::RasterizerContext(const std::shared_ptr<RasterizerEnvironment>& environment_, const std::shared_ptr<RasterizerSharedContext>& sharedContext_ /*= nullptr*/)
    : _d(new RasterizerContext_P(this))
    , environment(environment_)
    , sharedContext(sharedContext_)
{
}

OsmAnd::RasterizerContext::~RasterizerContext()
{
    _d->clear();
}
