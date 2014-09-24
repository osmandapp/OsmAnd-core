#ifndef _OSMAND_CORE_BASE_SEARCH_ENGINE_P_H_
#define _OSMAND_CORE_BASE_SEARCH_ENGINE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QSet>
#include <QReadWriteLock>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "ISearchEngine.h"
#include "BaseSearchEngine.h"

namespace OsmAnd
{
    class BaseSearchEngine_P
    {
        Q_DISABLE_COPY_AND_MOVE(BaseSearchEngine_P);

    public:
        typedef BaseSearchEngine::IDataSource IDataSource;

    private:
    protected:
        BaseSearchEngine_P(BaseSearchEngine* const owner);

        QSet< std::shared_ptr<const IDataSource> > _dataSources;
        mutable QReadWriteLock _dataSourcesLock;
    public:
        virtual ~BaseSearchEngine_P();

        ImplementationInterface<BaseSearchEngine> owner;

        void addDataSource(const std::shared_ptr<const IDataSource>& dataSource);
        bool removeDataSource(const std::shared_ptr<const IDataSource>& dataSource);
        unsigned int removeAllDataSources();
        QList< std::shared_ptr<const IDataSource> > getDataSources() const;
    };
}

#endif // !defined(_OSMAND_CORE_BASE_SEARCH_ENGINE_P_H_)