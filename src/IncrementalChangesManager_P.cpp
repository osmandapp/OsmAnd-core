#include "IncrementalChangesManager_P.h"
#include "IncrementalChangesManager.h"

#include "QtCommon.h"
#include "ignore_warnings_on_external_includes.h"
#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QBuffer>
#include "restore_internal_warnings.h"

#include "OsmAndCore_private.h"
#include "CoreResourcesEmbeddedBundle.h"
#include "ObfReader.h"
#include "ArchiveReader.h"
#include "ObfDataInterface.h"
#include "ResolvedMapStyle.h"
#include "UnresolvedMapStyle.h"
#include "OnlineTileSources.h"
#include "QKeyValueIterator.h"
#include "Logging.h"
#include "Utilities.h"
#include "CachedOsmandIndexes.h"
#include "ResourcesManager.h"

OsmAnd::IncrementalChangesManager_P::IncrementalChangesManager_P(
    IncrementalChangesManager* owner_,
    const std::shared_ptr<const IWebClient>& webClient_,
    ResourcesManager* resourcesManager_)
    : owner(owner_)
    , _webClient(webClient_)
    , _resourcesManager(resourcesManager_)
{
}

OsmAnd::IncrementalChangesManager_P::~IncrementalChangesManager_P()
{
}

void OsmAnd::IncrementalChangesManager_P::initialize()
{
}

bool OsmAnd::IncrementalChangesManager_P::addValidIncrementalUpdates(QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &liveResources,
                                                                     QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &mapResources)
{
    QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > result(mapResources);
    QHash<QString, uint64_t> regionMaps;
    for (const auto &res : mapResources)
    {
        if (!res || res->origin != ResourcesManager::ResourceOrigin::Installed)
            continue;
        
        const auto& installedResource = std::static_pointer_cast<const ResourcesManager::InstalledResource>(res);
        const uint64_t timestamp = installedResource->timestamp;
        QString regionName = QString(installedResource->id).remove(QStringLiteral(".map.obf"));
        regionMaps.insert(regionName, timestamp);
    }
    
    for (const auto &liveRes : liveResources)
    {
        if (!liveRes || liveRes->origin != ResourcesManager::ResourceOrigin::Installed)
            continue;
        
        const auto& liveResource = std::static_pointer_cast<const ResourcesManager::InstalledResource>(liveRes);
        QString regionName = QString(liveResource->id).remove(QRegExp(QStringLiteral("_([0-9]+_){2}[0-9]+\\.live\\.obf")));
        
        if (regionMaps.contains(regionName))
        {
            if (liveResource->timestamp > regionMaps.value(regionName))
                mapResources.insert(liveRes->id, qMove(liveRes));
            else
                _resourcesManager->uninstallResource(liveResource, liveRes);
        }
    }
    
    return true;
}


