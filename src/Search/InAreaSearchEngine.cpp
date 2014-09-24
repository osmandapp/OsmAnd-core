#include "InAreaSearchEngine.h"
#include "InAreaSearchEngine_P.h"

#include "InAreaSearchSession.h"

OsmAnd::InAreaSearchEngine::InAreaSearchEngine()
    : BaseSearchEngine(new InAreaSearchEngine_P(this))
    , _p(std::static_pointer_cast<InAreaSearchEngine_P>(BaseSearchEngine::_p.shared_ptr()))
{
}

OsmAnd::InAreaSearchEngine::~InAreaSearchEngine()
{
}

std::shared_ptr<OsmAnd::ISearchSession> OsmAnd::InAreaSearchEngine::createSession() const
{
    const std::shared_ptr<InAreaSearchSession> newSession(new InAreaSearchSession(getDataSources()));
    return newSession;
}
