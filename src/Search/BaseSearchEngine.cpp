#include "BaseSearchEngine.h"
#include "BaseSearchEngine_P.h"

OsmAnd::BaseSearchEngine::BaseSearchEngine(BaseSearchEngine_P* const p_)
    : _p(p_)
{
}

OsmAnd::BaseSearchEngine::~BaseSearchEngine()
{
}

void OsmAnd::BaseSearchEngine::addDataSource(const std::shared_ptr<const IDataSource>& dataSource)
{
    _p->addDataSource(dataSource);
}

bool OsmAnd::BaseSearchEngine::removeDataSource(const std::shared_ptr<const IDataSource>& dataSource)
{
    return _p->removeDataSource(dataSource);
}

unsigned int OsmAnd::BaseSearchEngine::removeAllDataSources()
{
    return _p->removeAllDataSources();
}

QList< std::shared_ptr<const OsmAnd::BaseSearchEngine::IDataSource> > OsmAnd::BaseSearchEngine::getDataSources() const
{
    return _p->getDataSources();
}
