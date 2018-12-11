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
        typedef IncrementalChangesManager::IncrementalUpdate IncrementalUpdate;
        typedef IncrementalChangesManager::RegionUpdateFiles RegionUpdateFiles;
        typedef IncrementalChangesManager::IncrementalUpdateGroupByMonth IncrementalUpdateGroupByMonth;
        typedef IncrementalChangesManager::IncrementalUpdateList IncrementalUpdateList;
        
    private:
        const std::shared_ptr<const IWebClient> _webClient;
        ResourcesManager* _resourcesManager;
        mutable QHash< QString, QList<std::shared_ptr<const ResourcesManager::LocalResource>> > _incrementalUpdatesResources;
        
        void onLocalResourcesChanged(const QList< QString >& added, const QList< QString >& removed);
        bool parseRepository(QXmlStreamReader& xmlReader,
                             QList< std::shared_ptr<const IncrementalUpdate> >& repository) const;
        bool getIncrementalUpdatesForRegion(
                                            const QString &region,
                                            uint64_t timestamp,
                                            QList< std::shared_ptr<const IncrementalUpdate> >& repository) const;
        const std::shared_ptr<const OsmAnd::ResourcesManager::InstalledResource> getInstalledResource(const QString &id) const;
        mutable QHash< QString, std::shared_ptr<RegionUpdateFiles> > _updatesStructure;
    protected:
        IncrementalChangesManager_P(
            IncrementalChangesManager* owner,
            const std::shared_ptr<const IWebClient>& webClient,
            ResourcesManager* resourcesManager);
        void initialize();
    public:
        virtual ~IncrementalChangesManager_P();

        ImplementationInterface<IncrementalChangesManager> owner;
        
        bool addValidIncrementalUpdates(QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &liveResources,
                                        QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &mapResources);
        std::shared_ptr<const IncrementalUpdateList> getUpdatesByMonth(const QString& regionName) const;
        
        void deleteUpdates(const QString &regionName);

    friend class OsmAnd::IncrementalChangesManager;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_P_H_)
