#include "InAreaSearchSession_P.h"
#include "InAreaSearchSession.h"

OsmAnd::InAreaSearchSession_P::InAreaSearchSession_P(InAreaSearchSession* const owner_)
    : BaseSearchSession_P(owner_)
    , owner(owner_)
{
}

OsmAnd::InAreaSearchSession_P::~InAreaSearchSession_P()
{
}

void OsmAnd::InAreaSearchSession_P::setArea(const AreaI64& area)
{
    int i = 5;
}

OsmAnd::AreaI64 OsmAnd::InAreaSearchSession_P::getArea() const
{
    return AreaI64();
}
