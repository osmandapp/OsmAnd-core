#ifndef _OSMAND_CORE_INCREMENTAL_CHANGES_MANAGER_H_
#define _OSMAND_CORE_INCREMENTAL_CHANGES_MANAGER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QList>
#include <QString>
#include <QStringList>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/Observable.h>
#include <OsmAndCore/IWebClient.h>
#include <OsmAndCore/WebClient.h>
#include <OsmAndCore/AccessLockCounter.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>
#include <OsmAndCore/ResourcesManager.h>

namespace OsmAnd
{
    class IncrementalChangesManager_P;
    class OSMAND_CORE_API IncrementalChangesManager
    {
        Q_DISABLE_COPY_AND_MOVE(IncrementalChangesManager);
    public:
        

    private:
        PrivateImplementation<IncrementalChangesManager_P> _p;
        
    protected:
        IncrementalChangesManager(
                                  const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient()),
                                  ResourcesManager* resourcesManager = nullptr);
    public:
        
        virtual ~IncrementalChangesManager();

        const QString repositoryBaseUrl;
        
        bool addValidIncrementalUpdates(QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &liveResources,
                                        QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &mapResources);
    friend class OsmAnd::ResourcesManager_P;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_H_)
