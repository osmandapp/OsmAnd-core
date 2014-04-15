#include "ResourcesManager_P.h"
#include "ResourcesManager.h"

#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>

#include "ObfReader.h"
#include "ArchiveReader.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::ResourcesManager_P::ResourcesManager_P(ResourcesManager* owner_)
    : owner(owner_)
{
}

OsmAnd::ResourcesManager_P::~ResourcesManager_P()
{
}

bool OsmAnd::ResourcesManager_P::rescanLocalStoragePaths() const
{
    // Collect resources from all local paths
    QHash< QString, std::shared_ptr<const LocalResource> > localResources;
    if(!rescanLocalStoragePath(owner->localStoragePath, localResources))
        return false;
    for(const auto& extraStoragePath : constOf(owner->extraStoragePaths))
    {
        if(!rescanLocalStoragePath(extraStoragePath, localResources))
            return false;
    }

    // Store results of rescan (thread-safe)
    {
        QWriteLocker scopedLocker(&_localResourcesLock);
        _localResources = localResources;
    }

    return true;
}

bool OsmAnd::ResourcesManager_P::rescanLocalStoragePath(const QString& storagePath, QHash< QString, std::shared_ptr<const LocalResource> >& outResult)
{
    const QDir storageDir(storagePath);

    // Find ResourceType::MapRegion -> "*.obf" files
    QFileInfoList mapRegionFiles;
    Utilities::findFiles(storageDir, QStringList() << QLatin1String("*.obf"), mapRegionFiles, false);
    for(const auto& mapRegionFile : constOf(mapRegionFiles))
    {
        const auto filePath = mapRegionFile.absoluteFilePath();

        // Read information from OBF
        std::shared_ptr<QFile> obfFile(new QFile(filePath));
        if(!obfFile->open(QIODevice::ReadOnly))
        {
            LogPrintf(LogSeverityLevel::Warning, "Failed to open '%s'", obfFile->fileName());
            continue;
        }
        const auto obfInfo = ObfReader(obfFile).obtainInfo();
        obfFile->close();
        obfFile.reset();

        // Create local resource entry
        const auto name = mapRegionFile.fileName();
        std::shared_ptr<LocalResource> localResource(new LocalObfResource(
            name,
            ResourceType::MapRegion,
            mapRegionFile.size(),
            filePath,
            obfInfo));
        outResult.insert(name, qMove(localResource));
    }

    // Find ResourceType::VoicePack -> "*.voice" directories
    QFileInfoList voicePackDirectories;
    Utilities::findDirectories(storageDir, QStringList() << QLatin1String("*.voice"), voicePackDirectories, false);

    return true;
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> > OsmAnd::ResourcesManager_P::getLocalResources() const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    return _localResources.values();
}

std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> OsmAnd::ResourcesManager_P::getLocalResource(const QString& name) const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    const auto citResource = _localResources.constFind(name);
    if(citResource == _localResources.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::refreshRepositoryIndex() const
{
    // Download content of the index
    std::shared_ptr<const WebClient::RequestResult> requestResult;
    const auto& downloadResult = _webClient.downloadData(QUrl(owner->repositoryBaseUrl + QLatin1String("/get_indexes.php")), &requestResult);
    if(downloadResult.isNull() || !requestResult->isSuccessful())
        return false;

    QList< std::shared_ptr<const ResourceInRepository> > resources;

    // Parse XML
    bool ok = false;
    QXmlStreamReader xmlReader(downloadResult);
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

        std::shared_ptr<ResourceInRepository> resourceInRepository(new ResourceInRepository(
            QString(name).replace(QLatin1String(".zip"), QString()),
            resourceType,
            timestamp,
            contentSize,
            owner->repositoryBaseUrl + QLatin1String("/download.php?file=") + QUrl::toPercentEncoding(name),
            containerSize));
        resources.push_back(qMove(resourceInRepository));
    }
    if(xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning, "XML error: %s (%d, %d)", qPrintable(xmlReader.errorString()), xmlReader.lineNumber(), xmlReader.columnNumber());
        return false;
    }

    // Save result
    {
        QWriteLocker scopedLocker(&_repositoryIndexLock);
        _repositoryIndex.clear();
        for(auto& entry : resources)
            _repositoryIndex.insert(entry->name, qMove(entry));
    }
    
    return true;
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager::ResourceInRepository> > OsmAnd::ResourcesManager_P::getRepositoryIndex() const
{
    QReadLocker scopedLocker(&_repositoryIndexLock);
    return _repositoryIndex.values();
}

std::shared_ptr<const OsmAnd::ResourcesManager::ResourceInRepository> OsmAnd::ResourcesManager_P::getResourceInRepository(const QString& name) const
{
    QReadLocker scopedLocker(&_repositoryIndexLock);

    const auto citResource = _repositoryIndex.constFind(name);
    if(citResource == _repositoryIndex.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::isResourceInstalled(const QString& name) const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    return (_localResources.constFind(name) != _localResources.cend());
}

bool OsmAnd::ResourcesManager_P::uninstallResource(const QString& name)
{
    QWriteLocker scopedLocker(&_localResourcesLock);

    const auto itResource = _localResources.find(name);
    if(itResource == _localResources.end())
        return false;

    const auto resource = *itResource;

    //TODO: lock obf file for removal
    if(const auto obfResource = std::dynamic_pointer_cast<const LocalObfResource>(resource))
    {
//        obfResource->obfFile->lockForRemoval();
    }

    // Erase from collection
    _localResources.erase(itResource);

    // Remove as file or as directory
    bool ok = false;
    if(resource->type == ResourceType::MapRegion)
        ok = QFile(resource->localPath).remove();
    /*else if(resource->type == ResourceType::VoicePack)
        ok = QDir(resource->localPath).removeRecursively();*/
    if(!ok)
        return false;
    return true;
}

bool OsmAnd::ResourcesManager_P::installFromFile(const QString& filePath, const ResourceType resourceType)
{
    return installFromFile(QFileInfo(filePath).fileName().replace(QLatin1String(".zip"), QString()), filePath, resourceType);
}

bool OsmAnd::ResourcesManager_P::installFromFile(const QString& name, const QString& filePath, const ResourceType resourceType)
{
    QWriteLocker scopedLocker(&_localResourcesLock);

    const auto itResource = _localResources.find(name);
    if(itResource != _localResources.end())
        return false;

    switch(resourceType)
    {
    case ResourceType::MapRegion:
        return installMapRegionFromFile(name, filePath);
    }

    return false;
}

bool OsmAnd::ResourcesManager_P::installMapRegionFromFile(const QString& name, const QString& filePath)
{
    ArchiveReader archive(filePath);

    // List items
    bool ok = false;
    const auto archiveItems = archive.getItems(&ok);
    if(!ok)
        return false;

    // Find the OBF file
    ArchiveReader::Item obfArchiveItem;
    for(const auto& archiveItem : constOf(archiveItems))
    {
        if(!archiveItem.isValid() || !archiveItem.name.endsWith(QLatin1String(".obf")))
            continue;

        obfArchiveItem = archiveItem;
        break;
    }
    if(!obfArchiveItem.isValid())
        return false;

    // Extract that file without keeping directory structure
    const auto localFileName = QDir(owner->localStoragePath).absoluteFilePath(name);
    if(!archive.extractItemToFile(obfArchiveItem.name, localFileName))
        return false;

    // Read information from OBF
    std::shared_ptr<QFile> obfFile(new QFile(localFileName));
    if(!obfFile->open(QIODevice::ReadOnly))
    {
        LogPrintf(LogSeverityLevel::Warning, "Failed to open '%s'", obfFile->fileName());
        obfFile->remove();
        return false;
    }
    const auto fileSize = obfFile->size();
    const auto obfInfo = ObfReader(obfFile).obtainInfo();
    obfFile->close();
    obfFile.reset();

    // Create local resource entry
    std::shared_ptr<LocalResource> localResource(new LocalObfResource(
        name,
        ResourceType::MapRegion,
        fileSize,
        localFileName,
        obfInfo));
    _localResources.insert(name, qMove(localResource));

    return true;
}

bool OsmAnd::ResourcesManager_P::installFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback)
{
    const auto& resource = getResourceInRepository(name);
    if(!resource)
        return false;

    const auto tmpFilePath = QDir(owner->localTemporaryPath).absoluteFilePath(QString("%1.%2")
        .arg(QString(QCryptographicHash::hash(name.toLocal8Bit(), QCryptographicHash::Md5).toHex()))
        .arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));

    bool ok = _webClient.downloadFile(resource->containerDownloadUrl, tmpFilePath, nullptr, downloadProgressCallback);
    if(!ok)
        return false;

    if(!installFromFile(name, tmpFilePath, resource->type))
    {
        QFile(tmpFilePath).remove();
        return false;
    }

    QFile(tmpFilePath).remove();
    return true;
}

bool OsmAnd::ResourcesManager_P::updateAvailableInRepositoryFor(const QString& name) const
{
    const auto& resourceInRepository = getResourceInRepository(name);
    if(!resourceInRepository)
        return false;
    const auto& localResource = getLocalResource(name);
    if(!localResource)
        return false;

    return (localResource->timestamp < resourceInRepository->timestamp);
}

QList<QString> OsmAnd::ResourcesManager_P::getAvailableUpdatesFromRepository() const
{
    QReadLocker scopedLocker2(&_localResourcesLock);

    QList<QString> resourcesWithUpdates;
    for(const auto& localResource : constOf(_localResources))
    {
        const auto& resourceInRepository = getResourceInRepository(localResource->name);
        if(!resourceInRepository)
            continue;

        if(localResource->timestamp < resourceInRepository->timestamp)
            resourcesWithUpdates.push_back(localResource->name);
    }

    return resourcesWithUpdates;
}

bool OsmAnd::ResourcesManager_P::updateFromFile(const QString& filePath)
{
    return updateFromFile(QFileInfo(filePath).fileName().replace(QLatin1String(".zip"), QString()), filePath);
}

bool OsmAnd::ResourcesManager_P::updateFromFile(const QString& name, const QString& filePath)
{
    return false;
}

bool OsmAnd::ResourcesManager_P::updateFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback)
{
    const auto& resource = getResourceInRepository(name);
    if(!resource)
        return false;

    const auto tmpFilePath = QDir(owner->localTemporaryPath).absoluteFilePath(QString("%1.%2")
        .arg(QString(QCryptographicHash::hash(name.toLocal8Bit(), QCryptographicHash::Md5).toHex()))
        .arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));

    bool ok = _webClient.downloadFile(resource->containerDownloadUrl, tmpFilePath, nullptr, downloadProgressCallback);
    if(!ok)
        return false;

    if(!updateFromFile(name, tmpFilePath))
    {
        QFile(tmpFilePath).remove();
        return false;
    }

    QFile(tmpFilePath).remove();
    return true;
}

std::shared_ptr<const OsmAnd::IObfsCollection> OsmAnd::ResourcesManager_P::getObfsCollection() const
{
    return std::shared_ptr<const OsmAnd::IObfsCollection>();
}
