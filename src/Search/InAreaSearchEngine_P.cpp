#include "InAreaSearchEngine_P.h"
#include "InAreaSearchEngine.h"

OsmAnd::InAreaSearchEngine_P::InAreaSearchEngine_P(InAreaSearchEngine* const owner_)
    : BaseSearchEngine_P(owner_)
    , owner(owner_)
{
}

OsmAnd::InAreaSearchEngine_P::~InAreaSearchEngine_P()
{
}
