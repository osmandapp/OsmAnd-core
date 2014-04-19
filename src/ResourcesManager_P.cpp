#include "ResourcesManager_P.h"
#include "ResourcesManager.h"

#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QTextStream>

#include "OsmAndCore_private.h"
#include "EmbeddedResources.h"
#include "ObfReader.h"
#include "ArchiveReader.h"
#include "ObfDataInterface.h"
#include "MapStylesPresets.h"
#include "OnlineTileSources.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::ResourcesManager_P::ResourcesManager_P(ResourcesManager* owner_)
    : owner(owner_)
    , _fileSystemWatcher(new QFileSystemWatcher())
    , _resourcesInRepositoryLoaded(false)
    , obfsCollection(new ObfsCollection(this))
{
    _fileSystemWatcher->moveToThread(gMainThread);
}

OsmAnd::ResourcesManager_P::~ResourcesManager_P()
{
    _fileSystemWatcher->deleteLater();
}

void OsmAnd::ResourcesManager_P::initialize()
{
    if(!owner->miniBasemapFilename.isNull())
    {
        const std::shared_ptr<const ObfFile> obfFile(new ObfFile(owner->miniBasemapFilename));
        if(ObfReader(obfFile).obtainInfo())
            _miniBasemapObfFile = obfFile;
        else
            LogPrintf(LogSeverityLevel::Warning, "Failed to open mini basemap '%s'", qPrintable(owner->miniBasemapFilename));
    }
}

void OsmAnd::ResourcesManager_P::attachToFileSystem()
{
    _onDirectoryChangedConnection = QObject::connect(
        _fileSystemWatcher, &QFileSystemWatcher::directoryChanged,
        (std::function<void(const QString&)>)std::bind(&ResourcesManager_P::onDirectoryChanged, this, std::placeholders::_1));
    _onFileChangedConnection = QObject::connect(
        _fileSystemWatcher, &QFileSystemWatcher::fileChanged,
        (std::function<void(const QString&)>)std::bind(&ResourcesManager_P::onFileChanged, this, std::placeholders::_1));

    for(const auto& extraStoragePath : constOf(owner->extraStoragePaths))
        _fileSystemWatcher->addPath(extraStoragePath);
}

void OsmAnd::ResourcesManager_P::detachFromFileSystem()
{
    _fileSystemWatcher->removePaths(_fileSystemWatcher->files());
    _fileSystemWatcher->removePaths(_fileSystemWatcher->directories());

    QObject::disconnect(_onDirectoryChangedConnection);
    QObject::disconnect(_onFileChangedConnection);
}

void OsmAnd::ResourcesManager_P::onDirectoryChanged(const QString& path)
{
    rescanLocalStoragePaths();
}

void OsmAnd::ResourcesManager_P::onFileChanged(const QString& path)
{
    rescanLocalStoragePaths();
}

void OsmAnd::ResourcesManager_P::inflateBuiltInResources()
{
    // Built-in presets for "default" map style
    std::shared_ptr<MapStylesPresets> defaultMapStylesPresets(new MapStylesPresets());
    defaultMapStylesPresets->loadFrom(EmbeddedResources::decompressResource(
        QLatin1String("map/mapStylesPresets/default.mapStylesPresets.xml")));
    std::shared_ptr<Resource> defaultMapStylesPresetsResource(new MapStylesPresetsResource(
        QLatin1String("default.mapStylesPresets.xml"),
        defaultMapStylesPresets));
    _builtinResources.insert(defaultMapStylesPresetsResource->id, defaultMapStylesPresetsResource);

    // Built-in online tile sources
    std::shared_ptr<Resource> defaultOnlineTileSourcesResource(new OnlineTileSourcesResource(
        QLatin1String("default.onlineTileSources.xml"),
        OnlineTileSources::getBuiltIn()));
    _builtinResources.insert(defaultOnlineTileSourcesResource->id, defaultOnlineTileSourcesResource);
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager_P::Resource> > OsmAnd::ResourcesManager_P::getBuiltInResources() const
{
    return _builtinResources.values();
}

std::shared_ptr<const OsmAnd::ResourcesManager_P::Resource> OsmAnd::ResourcesManager_P::getBuiltInResource(const QString& id) const
{
    const auto citResource = _builtinResources.constFind(id);
    if(citResource == _builtinResources.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::isBuiltInResource(const QString& id) const
{
    return _builtinResources.contains(id);
}

bool OsmAnd::ResourcesManager_P::rescanLocalStoragePaths() const
{
    QWriteLocker scopedLocker(&_localResourcesLock);

    QHash< QString, std::shared_ptr<const Resource> > localResources;
    if(!rescanLocalStoragePath(owner->localStoragePath, false, localResources))
        return false;
    for(const auto& extraStoragePath : constOf(owner->extraStoragePaths))
    {
        if(!rescanLocalStoragePath(extraStoragePath, true, localResources))
            return false;
    }
    _localResources = localResources;
    owner->localResourcesChangeObservable.notify(owner);

    return true;
}

bool OsmAnd::ResourcesManager_P::rescanLocalStoragePath(const QString& storagePath, const bool isExtraStorage, QHash< QString, std::shared_ptr<const Resource> >& outResult)
{
    const QDir storageDir(storagePath);

    // Find *.obf files, which may be MapRegion, RoadMapRegion or SrtmRegion
    QFileInfoList obfFileInfos;
    Utilities::findFiles(storageDir, QStringList() << QLatin1String("*.obf"), obfFileInfos, false);
    for(const auto& obfFileInfo : constOf(obfFileInfos))
    {
        const auto filePath = obfFileInfo.absoluteFilePath();

        // Read information from OBF
        const std::shared_ptr<const ObfFile> obfFile(new ObfFile(filePath));
        if(!ObfReader(obfFile).obtainInfo())
        {
            LogPrintf(LogSeverityLevel::Warning, "Failed to open '%s'", qPrintable(filePath));
            continue;
        }

        // Create local resource entry
        const auto name = obfFileInfo.fileName();
        std::shared_ptr<ResourceOrigin> resourceOrigin(new InstalledResourceOrigin(
            filePath,
            obfFileInfo.size(),
            obfFile->obfInfo->creationTimestamp));
        std::shared_ptr<Resource> localResource(new LocalObfResource(
            name,
            ResourceType::MapRegion,
            resourceOrigin,
            obfFile));
        outResult.insert(name, qMove(localResource));
    }

    // Find ResourceType::VoicePack -> "*.voice" directories
    QFileInfoList voicePackDirectories;
    Utilities::findDirectories(storageDir, QStringList() << QLatin1String("*.voice"), voicePackDirectories, false);
    for(const auto& voicePackDirectory : constOf(voicePackDirectories))
    {
        const auto dirPath = voicePackDirectory.absoluteFilePath();

        // Read special timestamp file
        uint64_t timestamp = 0;
        QFile timestampFile(QDir(dirPath).absoluteFilePath(QLatin1String(".timestamp")));
        if(timestampFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream(&timestampFile) >> timestamp;
            timestampFile.flush();
            timestampFile.close();
        }
        else
        {
            QFile voiceConfig(QDir(dirPath).absoluteFilePath(QLatin1String("_config.p")));
            if(voiceConfig.exists())
                timestamp = QFileInfo(voiceConfig).lastModified().toMSecsSinceEpoch();
        }

        // Read special size file
        uint64_t contentSize = 0;
        QFile sizeFile(QDir(dirPath).absoluteFilePath(QLatin1String(".size")));
        if(sizeFile.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            QTextStream(&sizeFile) >> contentSize;
            sizeFile.flush();
            sizeFile.close();
        }

        // Create local resource entry
        const auto name = voicePackDirectory.fileName();
        std::shared_ptr<ResourceOrigin> resourceOrigin(new InstalledResourceOrigin(
            dirPath,
            contentSize,
            timestamp));
        std::shared_ptr<Resource> localResource(new Resource(
            name,
            ResourceType::VoicePack, 
            resourceOrigin));
        outResult.insert(name, qMove(localResource));
    }

    // Find ResourceType::MapStylesPresetsResource -> "*.mapStylesPresets.xml" files (only in extra storage)
    if(isExtraStorage)
    {
        QFileInfoList mapStylesPresetsFileInfos;
        Utilities::findDirectories(storageDir, QStringList() << QLatin1String("*.mapStylesPresets.xml"), mapStylesPresetsFileInfos, false);
        for(const auto& mapStylesPresetsFileInfo : constOf(mapStylesPresetsFileInfos))
        {
            const auto fileName = mapStylesPresetsFileInfo.absoluteFilePath();
            const auto fileSize = mapStylesPresetsFileInfo.size();
            
            // Create local resource entry
            const auto name = mapStylesPresetsFileInfo.fileName();
            std::shared_ptr<ResourceOrigin> resourceOrigin(new UserResourceOrigin(
                fileName,
                fileSize,
                name));
            std::shared_ptr<Resource> localResource(new MapStylesPresetsResource(
                name,
                resourceOrigin));
            outResult.insert(name, qMove(localResource));
        }
    }

    // Find ResourceType::OnlineTileSourcesResource -> "*.onlineTileSources.xml" files (only in extra storage)
    if(isExtraStorage)
    {
        QFileInfoList onlineTileSourcesFileInfos;
        Utilities::findDirectories(storageDir, QStringList() << QLatin1String("*.onlineTileSources.xml"), onlineTileSourcesFileInfos, false);
        for(const auto& onlineTileSourcesFileInfo : constOf(onlineTileSourcesFileInfos))
        {
            const auto fileName = onlineTileSourcesFileInfo.absoluteFilePath();
            const auto fileSize = onlineTileSourcesFileInfo.size();

            // Create local resource entry
            const auto name = onlineTileSourcesFileInfo.fileName();
            std::shared_ptr<ResourceOrigin> resourceOrigin(new UserResourceOrigin(
                fileName,
                fileSize,
                name));
            std::shared_ptr<Resource> localResource(new OnlineTileSourcesResource(
                name,
                resourceOrigin));
            outResult.insert(name, qMove(localResource));
        }
    }

    return true;
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager::Resource> > OsmAnd::ResourcesManager_P::getLocalResources() const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    return _localResources.values();
}

std::shared_ptr<const OsmAnd::ResourcesManager::Resource> OsmAnd::ResourcesManager_P::getLocalResource(const QString& id) const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    const auto citResource = _localResources.constFind(id);
    if(citResource == _localResources.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::isLocalResource(const QString& id) const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    return _localResources.contains(id);
}

bool OsmAnd::ResourcesManager_P::parseRepository(QXmlStreamReader& xmlReader, QList< std::shared_ptr<const Resource> >& repository) const
{
    bool ok = false;
    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        if(!xmlReader.isStartElement())
            continue;
        const auto tagName = xmlReader.name();
        const auto& attribs = xmlReader.attributes();

        const auto& resourceTypeValue = attribs.value(QLatin1String("type"));
        if(resourceTypeValue.isNull())
            continue;
        const auto& nameValue = attribs.value(QLatin1String("name"));
        if(nameValue.isNull())
            continue;
        const auto& timestampValue = attribs.value(QLatin1String("timestamp"));
        if(timestampValue.isNull())
            continue;
        const auto& containerSizeValue = attribs.value(QLatin1String("containerSize"));
        if(containerSizeValue.isNull())
            continue;
        const auto& contentSizeValue = attribs.value(QLatin1String("contentSize"));
        if(contentSizeValue.isNull())
            continue;

        const auto name = nameValue.toString();

        auto resourceType = ResourceType::Unknown;
        if(resourceTypeValue == QLatin1String("map"))
            resourceType = ResourceType::MapRegion;
        if(resourceTypeValue == QLatin1String("voice"))
            resourceType = ResourceType::VoicePack;
        if(resourceType == ResourceType::Unknown)
        {
            LogPrintf(LogSeverityLevel::Warning, "Unknown resource type '%s' for '%s'", qPrintableRef(resourceTypeValue), qPrintable(name));
            continue;
        }

        const auto timestamp = timestampValue.toULongLong(&ok);
        if(!ok)
        {
            LogPrintf(LogSeverityLevel::Warning, "Invalid timestamp '%s' for '%s'", qPrintableRef(timestampValue), qPrintable(name));
            continue;
        }

        const auto containerSize = containerSizeValue.toULongLong(&ok);
        if(!ok)
        {
            LogPrintf(LogSeverityLevel::Warning, "Invalid container size '%s' for '%s'", qPrintableRef(containerSizeValue), qPrintable(name));
            continue;
        }

        const auto contentSize = contentSizeValue.toULongLong(&ok);
        if(!ok)
        {
            LogPrintf(LogSeverityLevel::Warning, "Invalid content size '%s' for '%s'", qPrintableRef(contentSizeValue), qPrintable(name));
            continue;
        }

        std::shared_ptr<ResourceOrigin> resourceOrigin(new RepositoryResourceOrigin(
            owner->repositoryBaseUrl + QLatin1String("/download.php?file=") + QUrl::toPercentEncoding(name),
            contentSize,
            timestamp,
            containerSize));
        std::shared_ptr<Resource> resource(new Resource(
            QString(name).replace(QLatin1String(".zip"), QString()),
            resourceType,
            resourceOrigin));
        repository.push_back(qMove(resource));
    }
    if(xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning, "XML error: %s (%d, %d)", qPrintable(xmlReader.errorString()), xmlReader.lineNumber(), xmlReader.columnNumber());
        return false;
    }

    return true;
}

void OsmAnd::ResourcesManager_P::loadRepositoryFromCache()
{
    QWriteLocker scopedLocker(&_resourcesInRepositoryLock);

    QList< std::shared_ptr<const Resource> > resources;
    QFile repositoryCache(QDir(owner->localStoragePath).absoluteFilePath("repository.cache.xml"));
    bool ok = false;
    if(repositoryCache.open(QIODevice::ReadOnly | QIODevice::Text))
        ok = parseRepository(QXmlStreamReader(&repositoryCache), resources);
    repositoryCache.close();
    if(!ok)
        return;

    _resourcesInRepository.clear();
    for(auto& entry : resources)
        _resourcesInRepository.insert(entry->id, qMove(entry));
    _resourcesInRepositoryLoaded = true;
}

bool OsmAnd::ResourcesManager_P::isRepositoryAvailable() const
{
    return _resourcesInRepositoryLoaded;
}

bool OsmAnd::ResourcesManager_P::updateRepository() const
{
    QWriteLocker scopedLocker(&_resourcesInRepositoryLock);

    // Download content of the index
    std::shared_ptr<const WebClient::RequestResult> requestResult;
    const auto& downloadResult = _webClient.downloadData(QUrl(owner->repositoryBaseUrl + QLatin1String("/get_indexes.php")), &requestResult);
    if(downloadResult.isNull() || !requestResult->isSuccessful())
        return false;

    // Parse XML
    QList< std::shared_ptr<const Resource> > resources;
    bool ok = parseRepository(QXmlStreamReader(downloadResult), resources);
    if(!ok)
        return false;

    // Save repository locally
    QFile repositoryCache(QDir(owner->localStoragePath).absoluteFilePath("repository.cache.xml"));
    if(repositoryCache.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        ok = repositoryCache.write(downloadResult);
        ok = repositoryCache.flush() && ok;
        repositoryCache.close();
        if(!ok)
            repositoryCache.remove();
    }

    // Update repository in memory
    _resourcesInRepository.clear();
    for(auto& entry : resources)
        _resourcesInRepository.insert(entry->id, qMove(entry));
    _resourcesInRepositoryLoaded = true;
    
    return true;
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager::Resource> > OsmAnd::ResourcesManager_P::getResourcesInRepository() const
{
    QReadLocker scopedLocker(&_resourcesInRepositoryLock);

    return _resourcesInRepository.values();
}

std::shared_ptr<const OsmAnd::ResourcesManager::Resource> OsmAnd::ResourcesManager_P::getResourceInRepository(const QString& id) const
{
    QReadLocker scopedLocker(&_resourcesInRepositoryLock);

    const auto citResource = _resourcesInRepository.constFind(id);
    if(citResource == _resourcesInRepository.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::isResourceInRepository(const QString& id) const
{
    QReadLocker scopedLocker(&_resourcesInRepositoryLock);

    return _resourcesInRepository.contains(id);
}

//bool OsmAnd::ResourcesManager_P::isResourceInstalled(const QString& name) const
//{
//    QReadLocker scopedLocker(&_localResourcesLock);
//
//    return (_localResources.constFind(name) != _localResources.cend());
//}
//
//bool OsmAnd::ResourcesManager_P::uninstallResource(const QString& name)
//{
//    QWriteLocker scopedLocker(&_localResourcesLock);
//
//    const auto itResource = _localResources.find(name);
//    if(itResource == _localResources.end())
//        return false;
//
//    const auto& resource = *itResource;
//    bool success;
//    switch(resource->type)
//    {
//        case ResourceType::MapRegion:
//            success = uninstallMapRegion(resource);
//            break;
//        case ResourceType::VoicePack:
//            success = uninstallVoicePack(resource);
//            break;
//        default:
//            return false;
//    }
//    if(!success)
//        return false;
//
//    _localResources.erase(itResource);
//    owner->localResourcesChangeObservable.notify(owner);
//
//    return true;
//}
//
//bool OsmAnd::ResourcesManager_P::uninstallMapRegion(const std::shared_ptr<const LocalResource>& localResource_)
//{
//    const auto& localResource = std::dynamic_pointer_cast<const LocalObfResource>(localResource_);
//
//    // This lock will never be released
//    localResource->obfFile->lockForWriting();
//
//    return QFile(localResource->localPath).remove();
//}
//
//bool OsmAnd::ResourcesManager_P::uninstallVoicePack(const std::shared_ptr<const LocalResource>& localResource)
//{
//    return QDir(localResource->localPath).removeRecursively();
//}
//
//bool OsmAnd::ResourcesManager_P::installFromFile(const QString& filePath, const ResourceType resourceType)
//{
//    const auto guessedResourceName = QFileInfo(filePath).fileName().replace(QLatin1String(".zip"), QString());
//    return installFromFile(guessedResourceName, filePath, resourceType);
//}
//
//bool OsmAnd::ResourcesManager_P::installFromFile(const QString& name, const QString& filePath, const ResourceType resourceType)
//{
//    QWriteLocker scopedLocker(&_localResourcesLock);
//
//    const auto itResource = _localResources.find(name);
//    if(itResource != _localResources.end())
//        return false;
//
//    bool ok = false;
//    switch(resourceType)
//    {
//    case ResourceType::MapRegion:
//        ok = installMapRegionFromFile(name, filePath);
//        break;
//    case ResourceType::VoicePack:
//        ok = installVoicePackFromFile(name, filePath);
//        break;
//    }
//    if(ok)
//        owner->localResourcesChangeObservable.notify(owner);
//
//    return ok;
//}
//
//bool OsmAnd::ResourcesManager_P::installMapRegionFromFile(const QString& name, const QString& filePath)
//{
//    ArchiveReader archive(filePath);
//
//    // List items
//    bool ok = false;
//    const auto archiveItems = archive.getItems(&ok);
//    if(!ok)
//        return false;
//
//    // Find the OBF file
//    ArchiveReader::Item obfArchiveItem;
//    for(const auto& archiveItem : constOf(archiveItems))
//    {
//        if(!archiveItem.isValid() || !archiveItem.name.endsWith(QLatin1String(".obf")))
//            continue;
//
//        obfArchiveItem = archiveItem;
//        break;
//    }
//    if(!obfArchiveItem.isValid())
//        return false;
//
//    // Extract that file without keeping directory structure
//    const auto localFileName = QDir(owner->localStoragePath).absoluteFilePath(name);
//    if(!archive.extractItemToFile(obfArchiveItem.name, localFileName))
//        return false;
//
//    // Read information from OBF
//    const std::shared_ptr<const ObfFile> obfFile(new ObfFile(localFileName));
//    if(!ObfReader(obfFile).obtainInfo())
//    {
//        LogPrintf(LogSeverityLevel::Warning, "Failed to open '%s'", qPrintable(localFileName));
//        QFile(filePath).remove();
//        return false;
//    }
//
//    // Create local resource entry
//    std::shared_ptr<LocalResource> localResource(new LocalObfResource(
//        name,
//        ResourceType::MapRegion,
//        obfFile));
//    _localResources.insert(name, qMove(localResource));
//
//    return true;
//}
//
//bool OsmAnd::ResourcesManager_P::installVoicePackFromFile(const QString& name, const QString& filePath)
//{
//    ArchiveReader archive(filePath);
//
//    // List items
//    bool ok = false;
//    const auto archiveItems = archive.getItems(&ok);
//    if(!ok)
//        return false;
//
//    // Verify voice pack
//    ArchiveReader::Item voicePackConfigItem;
//    for(const auto& archiveItem : constOf(archiveItems))
//    {
//        if(!archiveItem.isValid() || archiveItem.name != QLatin1String("_config.p"))
//            continue;
//
//        voicePackConfigItem = archiveItem;
//        break;
//    }
//    if(!voicePackConfigItem.isValid())
//        return false;
//
//    // Extract all files to local directory
//    const auto localDirectoryName = QDir(owner->localStoragePath).absoluteFilePath(name);
//    uint64_t contentSize = 0;
//    if(!archive.extractAllItemsTo(localDirectoryName, &contentSize))
//        return false;
//
//    // Create special timestamp file
//    QFile timestampFile(QDir(localDirectoryName).absoluteFilePath(QLatin1String(".timestamp")));
//    timestampFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
//    QTextStream(&timestampFile) << voicePackConfigItem.modificationTime.toMSecsSinceEpoch();
//    timestampFile.flush();
//    timestampFile.close();
//
//    // Create special size file
//    QFile sizeFile(QDir(localDirectoryName).absoluteFilePath(QLatin1String(".size")));
//    sizeFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
//    QTextStream(&sizeFile) << contentSize;
//    sizeFile.flush();
//    sizeFile.close();
//
//    // Create local resource entry
//    std::shared_ptr<LocalResource> localResource(new LocalResource(
//        name,
//        ResourceType::VoicePack,
//        voicePackConfigItem.modificationTime.toMSecsSinceEpoch(),
//        contentSize,
//        localDirectoryName));
//    _localResources.insert(name, qMove(localResource));
//
//    return true;
//}
//
//bool OsmAnd::ResourcesManager_P::installFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback)
//{
//    if(isResourceInstalled(name))
//        return false;
//
//    const auto& resource = getResourceInRepository(name);
//    if(!resource)
//        return false;
//
//    const auto tmpFilePath = QDir(owner->localTemporaryPath).absoluteFilePath(QString("%1.%2")
//        .arg(QString(QCryptographicHash::hash(name.toLocal8Bit(), QCryptographicHash::Md5).toHex()))
//        .arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
//
//    bool ok = _webClient.downloadFile(resource->containerDownloadUrl, tmpFilePath, nullptr, downloadProgressCallback);
//    if(!ok)
//        return false;
//
//    if(!installFromFile(name, tmpFilePath, resource->type))
//    {
//        QFile(tmpFilePath).remove();
//        return false;
//    }
//
//    QFile(tmpFilePath).remove();
//    return true;
//}
//
//bool OsmAnd::ResourcesManager_P::updateAvailableInRepositoryFor(const QString& name) const
//{
//    const auto& resourceInRepository = getResourceInRepository(name);
//    if(!resourceInRepository)
//        return false;
//    const auto& localResource = getLocalResource(name);
//    if(!localResource)
//        return false;
//
//    return (localResource->timestamp < resourceInRepository->timestamp);
//}
//
//QList<QString> OsmAnd::ResourcesManager_P::getAvailableUpdatesFromRepository() const
//{
//    QReadLocker scopedLocker(&_localResourcesLock);
//
//    QList<QString> resourcesWithUpdates;
//    for(const auto& localResource : constOf(_localResources))
//    {
//        const auto& resourceInRepository = getResourceInRepository(localResource->name);
//        if(!resourceInRepository)
//            continue;
//
//        if(localResource->timestamp < resourceInRepository->timestamp)
//            resourcesWithUpdates.push_back(localResource->name);
//    }
//
//    return resourcesWithUpdates;
//}
//
//bool OsmAnd::ResourcesManager_P::updateFromFile(const QString& filePath)
//{
//    return updateFromFile(QFileInfo(filePath).fileName().replace(QLatin1String(".zip"), QString()), filePath);
//}
//
//bool OsmAnd::ResourcesManager_P::updateFromFile(const QString& name, const QString& filePath)
//{
//    QWriteLocker scopedLocker(&_localResourcesLock);
//
//    const auto itResource = _localResources.find(name);
//    if(itResource != _localResources.end())
//        return false;
//    const auto localResource = *itResource;
//
//    bool ok = false;
//    switch(localResource->type)
//    {
//    case ResourceType::MapRegion:
//        if(!uninstallMapRegion(localResource))
//            return false;
//        ok = installMapRegionFromFile(localResource->name, filePath);
//        break;
//    case ResourceType::VoicePack:
//        if(!uninstallVoicePack(localResource))
//            return false;
//        ok = installVoicePackFromFile(localResource->name, filePath);
//        break;
//    }
//    if(ok)
//        owner->localResourcesChangeObservable.notify(owner);
//
//    return ok;
//}
//
//bool OsmAnd::ResourcesManager_P::updateFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback)
//{
//    const auto& resource = getResourceInRepository(name);
//    if(!resource)
//        return false;
//
//    const auto tmpFilePath = QDir(owner->localTemporaryPath).absoluteFilePath(QString("%1.%2")
//        .arg(QString(QCryptographicHash::hash(name.toLocal8Bit(), QCryptographicHash::Md5).toHex()))
//        .arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));
//
//    bool ok = _webClient.downloadFile(resource->containerDownloadUrl, tmpFilePath, nullptr, downloadProgressCallback);
//    if(!ok)
//        return false;
//
//    if(!updateFromFile(name, tmpFilePath))
//    {
//        QFile(tmpFilePath).remove();
//        return false;
//    }
//
//    QFile(tmpFilePath).remove();
//    return true;
//}

OsmAnd::ResourcesManager_P::ObfsCollection::ObfsCollection(ResourcesManager_P* owner_)
    : owner(owner_)
{
}

OsmAnd::ResourcesManager_P::ObfsCollection::~ObfsCollection()
{
}

QVector< std::shared_ptr<const OsmAnd::ObfFile> > OsmAnd::ResourcesManager_P::ObfsCollection::getObfFiles() const
{
    QReadLocker scopedLocker(&owner->_localResourcesLock);

    bool otherBasemapPresent = false;
    QVector< std::shared_ptr<const ObfFile> > obfFiles;
    for(const auto& localResource : constOf(owner->_localResources))
    {
        if(localResource->type != ResourceType::MapRegion)
            continue;

        const auto& obfLocalResource = std::static_pointer_cast<const LocalObfResource>(localResource);
        if(obfLocalResource->obfFile->obfInfo->isBasemap)
            otherBasemapPresent = true;
        obfFiles.push_back(obfLocalResource->obfFile);
    }
    if(!otherBasemapPresent && owner->_miniBasemapObfFile)
        obfFiles.push_back(owner->_miniBasemapObfFile);

    return obfFiles;
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ResourcesManager_P::ObfsCollection::obtainDataInterface() const
{
    QReadLocker scopedLocker(&owner->_localResourcesLock);

    bool otherBasemapPresent = false;
    QList< std::shared_ptr<const ObfReader> > obfReaders;
    for(const auto& localResource : constOf(owner->_localResources))
    {
        if(localResource->type != ResourceType::MapRegion)
            continue;

        const auto& obfLocalResource = std::static_pointer_cast<const LocalObfResource>(localResource);
        if(!obfLocalResource->obfFile->tryLockForReading())
            continue;
        if(obfLocalResource->obfFile->obfInfo->isBasemap)
            otherBasemapPresent = true;
        std::shared_ptr<const ObfReader> obfReader(new ObfReader(obfLocalResource->obfFile, false));
        obfReaders.push_back(qMove(obfReader));
    }
    if(!otherBasemapPresent && owner->_miniBasemapObfFile)
    {
        std::shared_ptr<const ObfReader> obfReader(new ObfReader(owner->_miniBasemapObfFile));
        obfReaders.push_back(qMove(obfReader));
    }

    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface(obfReaders));
}
