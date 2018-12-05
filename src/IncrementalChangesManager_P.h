#ifndef _OSMAND_CORE_INCREMENTAL_CHANGES_MANAGER_P_H_
#define _OSMAND_CORE_INCREMENTAL_CHANGES_MANAGER_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QList>
#include <QHash>
#include <QString>
#include <QReadWriteLock>
#include <QFileSystemWatcher>
#include <QXmlStreamReader>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "IWebClient.h"
#include "IncrementalChangesManager.h"

namespace OsmAnd
{
    class ResourcesManager;
    class CachedOsmandIndexes;
    
    class IncrementalChangesManager_P Q_DECL_FINAL
    {
    public:
        bool addValidIncrementalUpdates(QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &liveResources,
                                        QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &mapResources);
    private:
        const std::shared_ptr<const IWebClient> _webClient;
        ResourcesManager* _resourcesManager;
        
        mutable QHash< QString, QList<std::shared_ptr<const ResourcesManager::LocalResource>> > _incrementalUpdatesResources;
    protected:
        IncrementalChangesManager_P(
            IncrementalChangesManager* owner,
            const std::shared_ptr<const IWebClient>& webClient,
            ResourcesManager* resourcesManager);
        void initialize();
    public:
        virtual ~IncrementalChangesManager_P();

        ImplementationInterface<IncrementalChangesManager> owner;

    friend class OsmAnd::IncrementalChangesManager;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_P_H_)
