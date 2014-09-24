#include "InAreaSearchSession.h"
#include "InAreaSearchSession_P.h"

OsmAnd::InAreaSearchSession::InAreaSearchSession(const QList< std::shared_ptr<const ISearchEngine::IDataSource> >& dataSources)
    : BaseSearchSession(new InAreaSearchSession_P(this), dataSources)
    , _p(std::static_pointer_cast<InAreaSearchSession_P>(BaseSearchSession::_p.shared_ptr()))
{
}

OsmAnd::InAreaSearchSession::~InAreaSearchSession()
{
}

void OsmAnd::InAreaSearchSession::setArea(const AreaI64& area)
{
    _p->setArea(area);
}

OsmAnd::AreaI64 OsmAnd::InAreaSearchSession::getArea() const
{
    return _p->getArea();
}
