#include "RasterizerContext.h"
#include "RasterizerContext_P.h"

OsmAnd::RasterizerContext::RasterizerContext(const std::shared_ptr<RasterizerEnvironment>& environment_)
    : _d(new RasterizerContext_P(this))
    , environment(environment_)
{
}

OsmAnd::RasterizerContext::~RasterizerContext()
{
}

int OsmAnd::RasterizerContext::getSymbolsCount() const
{
    return _d->_symbols.size();
}
