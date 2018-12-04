#include "IncrementalChangesManager.h"
#include "IncrementalChangesManager_P.h"

#include <QStandardPaths>
#include <QDir>

#include "ObfInfo.h"

OsmAnd::IncrementalChangesManager::IncrementalChangesManager(
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/,
    const std::shared_ptr<ResourcesManager>& resourcesManager)
    : _p(new IncrementalChangesManager_P(this, webClient, resourcesManager))
    , repositoryBaseUrl(QStringLiteral("http://download.osmand.net"))
{

    _p->initialize();
}

OsmAnd::IncrementalChangesManager::~IncrementalChangesManager()
{
}

bool OsmAnd::IncrementalChangesManager::addValidIncrementalUpdates(QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &liveResources,
                                                                   QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &mapResources)
{
    return _p->addValidIncrementalUpdates(liveResources, mapResources);
}
