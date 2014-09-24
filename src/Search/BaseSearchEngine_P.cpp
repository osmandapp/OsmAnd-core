#include "BaseSearchEngine_P.h"
#include "BaseSearchEngine.h"

OsmAnd::BaseSearchEngine_P::BaseSearchEngine_P(BaseSearchEngine* const owner_)
    : owner(owner_)
{
}

OsmAnd::BaseSearchEngine_P::~BaseSearchEngine_P()
{
}

void OsmAnd::BaseSearchEngine_P::addDataSource(const std::shared_ptr<const IDataSource>& dataSource)
{
    QWriteLocker scopedLocker(&_dataSourcesLock);

    _dataSources.insert(dataSource);
}

bool OsmAnd::BaseSearchEngine_P::removeDataSource(const std::shared_ptr<const IDataSource>& dataSource)
{
    QWriteLocker scopedLocker(&_dataSourcesLock);

    return _dataSources.remove(dataSource);
}

unsigned int OsmAnd::BaseSearchEngine_P::removeAllDataSources()
{
    QWriteLocker scopedLocker(&_dataSourcesLock);

    const auto removedCount = _dataSources.size();
    _dataSources.clear();
    return removedCount;
}

QList< std::shared_ptr<const OsmAnd::BaseSearchEngine_P::IDataSource> > OsmAnd::BaseSearchEngine_P::getDataSources() const
{
    QReadLocker scopedLocker(&_dataSourcesLock);

    return _dataSources.toList();
}
