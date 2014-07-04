#include "ResourcesManager_P.h"
#include "ResourcesManager.h"

#include "QtCommon.h"
#include <QXmlStreamReader>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QBuffer>

#include "OsmAndCore_private.h"
#include "EmbeddedResources.h"
#include "ObfReader.h"
#include "ArchiveReader.h"
#include "ObfDataInterface.h"
#include "MapStyle.h"
#include "MapStylesPresetsCollection.h"
#include "OnlineTileSources.h"
#include "QKeyValueIterator.h"
#include "Logging.h"
#include "Utilities.h"

OsmAnd::ResourcesManager_P::ResourcesManager_P(ResourcesManager* owner_)
    : owner(owner_)
    , _fileSystemWatcher(new QFileSystemWatcher())
    , _localResourcesLock(QReadWriteLock::Recursive)
    , _resourcesInRepositoryLoaded(false)
    , onlineTileSources(new OnlineTileSourcesProxy(this))
    , mapStylesCollection(new MapStylesCollectionProxy(this))
    , mapStylesPresetsCollection(new MapStylesPresetsCollectionProxy(this))
    , obfsCollection(new ObfsCollectionProxy(this))
{
    _fileSystemWatcher->moveToThread(gMainThread);
}

OsmAnd::ResourcesManager_P::~ResourcesManager_P()
{
    _fileSystemWatcher->deleteLater();
}

void OsmAnd::ResourcesManager_P::initialize()
{
    if (!owner->miniBasemapFilename.isNull())
    {
        const std::shared_ptr<const ObfFile> obfFile(new ObfFile(owner->miniBasemapFilename));
        if (ObfReader(obfFile).obtainInfo())
            _miniBasemapObfFile = obfFile;
        else
            LogPrintf(LogSeverityLevel::Warning, "Failed to open mini basemap OBF '%s'", qPrintable(owner->miniBasemapFilename));
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

    if (!owner->userStoragePath.isNull())
        _fileSystemWatcher->addPath(owner->userStoragePath);
    for(const auto& readonlyExternalStoragePath : constOf(owner->readonlyExternalStoragePaths))
        _fileSystemWatcher->addPath(readonlyExternalStoragePath);
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
    rescanUnmanagedStoragePaths();
}

void OsmAnd::ResourcesManager_P::onFileChanged(const QString& path)
{
    rescanUnmanagedStoragePaths();
}

void OsmAnd::ResourcesManager_P::inflateBuiltInResources()
{
    bool ok;

    // Built-in map style
    auto defaultMapStyleContent = EmbeddedResources::decompressResource(
        QLatin1String("map/styles/default.render.xml"));
    std::shared_ptr<MapStyle> defaultMapStyle(new MapStyle(
        mapStylesCollection.get(),
        QLatin1String("default"),
        std::shared_ptr<QIODevice>(new QBuffer(&defaultMapStyleContent))));
    ok = defaultMapStyle->loadMetadata() && defaultMapStyle->load();
    assert(ok);
    std::shared_ptr<const BuiltinResource> defaultMapStyleResource(new BuiltinResource(
        QLatin1String("default.render.xml"),
        ResourceType::MapStyle,
        std::shared_ptr<const Resource::Metadata>(new MapStyleMetadata(defaultMapStyle))));
    _builtinResources.insert(defaultMapStyleResource->id, defaultMapStyleResource);

    // Built-in presets for "default" map style
    std::shared_ptr<MapStylesPresetsCollection> defaultMapStylesPresets(new MapStylesPresetsCollection());
    defaultMapStylesPresets->loadFrom(EmbeddedResources::decompressResource(
        QLatin1String("map/presets/default.map_styles_presets.xml")));
    std::shared_ptr<const BuiltinResource> defaultMapStylesPresetsResource(new BuiltinResource(
        QLatin1String("default.map_styles_presets.xml"),
        ResourceType::MapStylesPresets,
        std::shared_ptr<const Resource::Metadata>(new MapStylesPresetsMetadata(defaultMapStylesPresets))));
    _builtinResources.insert(defaultMapStylesPresetsResource->id, defaultMapStylesPresetsResource);

    // Built-in online tile sources
    std::shared_ptr<const BuiltinResource> defaultOnlineTileSourcesResource(new BuiltinResource(
        QLatin1String("default.online_tile_sources.xml"),
        ResourceType::OnlineTileSources,
        std::shared_ptr<const Resource::Metadata>(new OnlineTileSourcesMetadata(OnlineTileSources::getBuiltIn()))));
    _builtinResources.insert(defaultOnlineTileSourcesResource->id, defaultOnlineTileSourcesResource);
}

std::shared_ptr<const OsmAnd::ResourcesManager_P::Resource> OsmAnd::ResourcesManager_P::getResource(const QString& id) const
{
    // Check in builtin resources
    const auto citBuiltinResource = _builtinResources.constFind(id);
    if (citBuiltinResource != _builtinResources.cend())
        return *citBuiltinResource;

    // Check in local resources
    {
        QReadLocker scopedLocker(&_localResourcesLock);

        const auto citLocalResource = _localResources.constFind(id);
        if (citLocalResource != _localResources.cend())
            return *citLocalResource;
    }

    // Check in repository
    {
        QReadLocker scopedLocker(&_resourcesInRepositoryLock);

        const auto citResourceInRepository = _resourcesInRepository.constFind(id);
        if (citResourceInRepository != _resourcesInRepository.cend())
            return *citResourceInRepository;
    }

    return nullptr;
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager_P::BuiltinResource> > OsmAnd::ResourcesManager_P::getBuiltInResources() const
{
    return _builtinResources;
}

std::shared_ptr<const OsmAnd::ResourcesManager_P::BuiltinResource> OsmAnd::ResourcesManager_P::getBuiltInResource(const QString& id) const
{
    const auto citResource = _builtinResources.constFind(id);
    if (citResource == _builtinResources.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::isBuiltInResource(const QString& id) const
{
    return _builtinResources.contains(id);
}

bool OsmAnd::ResourcesManager_P::scanManagedStoragePath()
{
    QWriteLocker scopedLocker(&_localResourcesLock);

    assert(_localResources.isEmpty());
    if (!loadLocalResourcesFromPath(owner->localStoragePath, false, _localResources))
        return false;

    return true;
}

bool OsmAnd::ResourcesManager_P::rescanUnmanagedStoragePaths() const
{
    QWriteLocker scopedLocker(&_localResourcesLock);

    QHash< QString, std::shared_ptr<const LocalResource> > unmanagedResources;
    if (!owner->userStoragePath.isNull())
    {
        if (!loadLocalResourcesFromPath(owner->userStoragePath, true, unmanagedResources))
            return false;
    }
    for(const auto& readonlyExternalStoragePath : constOf(owner->readonlyExternalStoragePaths))
    {
        if (!loadLocalResourcesFromPath(readonlyExternalStoragePath, true, unmanagedResources))
            return false;
    }

    // Merge results with current resources
    QList< QString > addedResources;
    QList< QString > removedResources;
    QList< QString > updatedResources;
    auto itLocalResourceEntry = mutableIteratorOf(_localResources);
    while(itLocalResourceEntry.hasNext())
    {
        const auto& localResourceEntry = itLocalResourceEntry.next();
        const auto& id = localResourceEntry.key();
        auto& resource = localResourceEntry.value();

        // If this resource origin is not unmanaged, skip it
        if (resource->origin != ResourceOrigin::Unmanaged)
            continue;
        const auto itNewResource = unmanagedResources.find(id);

        // If this resource is gone, just copy it to removed and remove from collection
        if (itNewResource == unmanagedResources.end())
        {
            removedResources.push_back(id);
            itLocalResourceEntry.remove();
            continue;
        }

        // Check if type is the same
        const auto& newResource = *itNewResource;
        if (resource->type != newResource->type)
        {
            removedResources.push_back(id);
            itLocalResourceEntry.remove();
            continue;
        }

        // It's impossible to determine if resource was updated,
        // so assume that all resources that are still present have been updated
        resource = newResource;
        updatedResources.push_back(id);
        unmanagedResources.erase(itNewResource);
    }
    for(const auto& newResource : constOf(unmanagedResources))
    {
        const auto& id = newResource->id;
        if (_localResources.contains(id))
        {
            LogPrintf(LogSeverityLevel::Warning, "Ignoring new duplicate resource '%s'", qPrintable(id));
            continue;
        }

        _localResources.insert(id, newResource);
        addedResources.push_back(id);
    }

    scopedLocker.unlock();
    owner->localResourcesChangeObservable.postNotify(owner, addedResources, removedResources, updatedResources);

    return true;
}

bool OsmAnd::ResourcesManager_P::loadLocalResourcesFromPath(
    const QString& storagePath,
    const bool isUnmanagedStorage,
    QHash< QString, std::shared_ptr<const LocalResource> >& outResult) const
{
    const QDir storageDir(storagePath);

    // Find ResourceType::MapRegion -> "*.map.obf" files
    if (!isUnmanagedStorage)
    {
        QFileInfoList obfFileInfos;
        Utilities::findFiles(storageDir, QStringList() << QLatin1String("*.map.obf"), obfFileInfos, false);
        for(const auto& obfFileInfo : constOf(obfFileInfos))
        {
            const auto filePath = obfFileInfo.absoluteFilePath();

            // Read information from OBF
            const std::shared_ptr<const ObfFile> obfFile(new ObfFile(filePath));
            if (!ObfReader(obfFile).obtainInfo())
            {
                LogPrintf(LogSeverityLevel::Warning, "Failed to open OBF '%s'", qPrintable(filePath));
                continue;
            }

            // Create local resource entry
            const auto fileName = obfFileInfo.fileName();
            const auto resourceId = fileName.toLower();
            const auto pLocalResource = new InstalledResource(
                resourceId,
                ResourceType::MapRegion,
                filePath,
                obfFileInfo.size(),
                obfFile->obfInfo->creationTimestamp);
            pLocalResource->_metadata.reset(new ObfMetadata(obfFile));
            std::shared_ptr<const LocalResource> localResource(pLocalResource);
            outResult.insert(resourceId, qMove(localResource));
        }
    }

    // In unmanaged storage, "*.obf" files can contain anything anything
    if (isUnmanagedStorage)
    {
        QFileInfoList obfFileInfos;
        Utilities::findFiles(storageDir, QStringList() << QLatin1String("*.obf"), obfFileInfos, false);
        for(const auto& obfFileInfo : constOf(obfFileInfos))
        {
            const auto filePath = obfFileInfo.absoluteFilePath();
            const auto fileName = obfFileInfo.fileName();
            
            // Read information from OBF
            const std::shared_ptr<const ObfFile> obfFile(new ObfFile(filePath));
            const auto obfInfo = ObfReader(obfFile).obtainInfo();
            if (!obfInfo)
            {
                LogPrintf(LogSeverityLevel::Warning, "Failed to open OBF '%s'", qPrintable(filePath));
                continue;
            }

            // Determine resource type and id
            QString resourceId;
            auto resourceType = ResourceType::Unknown;
            if (!obfInfo->mapSections.isEmpty() &&
                !obfInfo->addressSections.isEmpty() &&
                !obfInfo->routingSections.isEmpty() &&
                !obfInfo->poiSections.isEmpty() &&
                !obfInfo->transportSections.isEmpty())
            {
                resourceType = ResourceType::MapRegion;
                resourceId = fileName.toLower().remove("_2")
                    .replace(QLatin1String(".obf"), QLatin1String(".map.obf"));
            }
            if (obfInfo->isBasemap &&
                !obfInfo->mapSections.isEmpty())
            {
                resourceType = ResourceType::MapRegion;
                resourceId = QLatin1String("world_basemap.map.obf");
            }
            if (resourceType == ResourceType::Unknown)
            {
                LogPrintf(LogSeverityLevel::Warning, "Failed to determine type of OBF '%s'", qPrintable(filePath));
                continue;
            }

            // Create local resource entry
            const auto pLocalResource = new InstalledResource(
                resourceId,
                resourceType,
                filePath,
                obfFileInfo.size(),
                obfFile->obfInfo->creationTimestamp);
            pLocalResource->_metadata.reset(new ObfMetadata(obfFile));
            std::shared_ptr<const LocalResource> localResource(pLocalResource);
            outResult.insert(resourceId, qMove(localResource));
        }
    }

    // Find ResourceType::VoicePack -> "*.voice" directories
    if (!isUnmanagedStorage)
    {
        QFileInfoList voicePackDirectories;
        Utilities::findDirectories(storageDir, QStringList() << QLatin1String("*.voice"), voicePackDirectories, false);
        for(const auto& voicePackDirectory : constOf(voicePackDirectories))
        {
            const auto dirPath = voicePackDirectory.absoluteFilePath();

            // Check for proper voice-pack
            QFile voiceConfig(QDir(dirPath).absoluteFilePath(QLatin1String("_config.p")));
            if (!voiceConfig.exists())
            {
                LogPrintf(LogSeverityLevel::Warning, "Failed to recognize voice-pack '%s'", qPrintable(dirPath));
                continue;
            }

            // Read special timestamp file
            uint64_t timestamp = 0;
            QFile timestampFile(QDir(dirPath).absoluteFilePath(QLatin1String(".timestamp")));
            if (timestampFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream(&timestampFile) >> timestamp;
                timestampFile.flush();
                timestampFile.close();
            }
            else
            {
                timestamp = QFileInfo(voiceConfig).lastModified().toMSecsSinceEpoch();
            }

            // Read special size file
            uint64_t contentSize = 0;
            QFile sizeFile(QDir(dirPath).absoluteFilePath(QLatin1String(".size")));
            if (sizeFile.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                QTextStream(&sizeFile) >> contentSize;
                sizeFile.flush();
                sizeFile.close();
            }

            // Create local resource entry
            const auto fileName = voicePackDirectory.fileName();
            const auto resourceId = fileName.toLower();
            std::shared_ptr<const LocalResource> localResource(new InstalledResource(
                resourceId,
                ResourceType::VoicePack,
                dirPath,
                contentSize,
                timestamp));
            outResult.insert(resourceId, qMove(localResource));
        }
    }

    // Find ResourceType::MapStyleResource -> "*.render.xml" files (only in unmanaged storage)
    if (isUnmanagedStorage)
    {
        QFileInfoList mapStyleFileInfos;
        Utilities::findFiles(storageDir, QStringList() << QLatin1String("*.render.xml"), mapStyleFileInfos, false);
        for(const auto& mapStyleFileInfo : constOf(mapStyleFileInfos))
        {
            const auto filePath = mapStyleFileInfo.absoluteFilePath();
            const auto fileSize = mapStyleFileInfo.size();

            // Load resource
            const std::shared_ptr<MapStyle> mapStyle(new MapStyle(mapStylesCollection.get(), filePath));
            if (!mapStyle->loadMetadata())
            {
                LogPrintf(LogSeverityLevel::Warning, "Failed to open map style '%s'", qPrintable(filePath));
                continue;
            }

            // Create local resource entry
            const auto fileName = mapStyleFileInfo.fileName();
            const auto resourceId = fileName.toLower();
            const auto pLocalResource = new UnmanagedResource(
                resourceId,
                ResourceType::MapStyle,
                filePath,
                fileSize,
                fileName);
            pLocalResource->_metadata.reset(new MapStyleMetadata(mapStyle));
            std::shared_ptr<const LocalResource> localResource(pLocalResource);
            outResult.insert(resourceId, qMove(localResource));
        }
    }

    // Find ResourceType::MapStylesPresetsResource -> "*.map_styles_presets.xml" files (only in unmanaged storage)
    if (isUnmanagedStorage)
    {
        QFileInfoList mapStylesPresetsFileInfos;
        Utilities::findDirectories(storageDir, QStringList() << QLatin1String("*.map_styles_presets.xml"), mapStylesPresetsFileInfos, false);
        for(const auto& mapStylesPresetsFileInfo : constOf(mapStylesPresetsFileInfos))
        {
            const auto filePath = mapStylesPresetsFileInfo.absoluteFilePath();
            const auto fileSize = mapStylesPresetsFileInfo.size();

            // Load resource
            const std::shared_ptr<MapStylesPresetsCollection> presets(new MapStylesPresetsCollection());
            if (!presets->loadFrom(filePath))
            {
                LogPrintf(LogSeverityLevel::Warning, "Failed to load map styles presets from '%s'", qPrintable(filePath));
                continue;
            }

            // Create local resource entry
            const auto fileName = mapStylesPresetsFileInfo.fileName();
            const auto resourceId = fileName.toLower();
            const auto pLocalResource = new UnmanagedResource(
                resourceId,
                ResourceType::MapStylesPresets,
                filePath,
                fileSize,
                fileName);
            pLocalResource->_metadata.reset(new MapStylesPresetsMetadata(presets));
            std::shared_ptr<const LocalResource> localResource(pLocalResource);
            outResult.insert(resourceId, qMove(localResource));
        }
    }

    // Find ResourceType::OnlineTileSourcesResource -> "*.online_tile_sources.xml" files (only in unmanaged storage)
    if (isUnmanagedStorage)
    {
        QFileInfoList onlineTileSourcesFileInfos;
        Utilities::findDirectories(storageDir, QStringList() << QLatin1String("*.online_tile_sources.xml"), onlineTileSourcesFileInfos, false);
        for(const auto& onlineTileSourcesFileInfo : constOf(onlineTileSourcesFileInfos))
        {
            const auto filePath = onlineTileSourcesFileInfo.absoluteFilePath();
            const auto fileSize = onlineTileSourcesFileInfo.size();

            // Load resource
            const std::shared_ptr<OnlineTileSources> sources(new OnlineTileSources());
            if (!sources->loadFrom(filePath))
            {
                LogPrintf(LogSeverityLevel::Warning, "Failed to load online tile sources from '%s'", qPrintable(filePath));
                continue;
            }

            // Create local resource entry
            const auto fileName = onlineTileSourcesFileInfo.fileName();
            const auto resourceId = fileName.toLower();
            const auto pLocalResource = new UnmanagedResource(
                resourceId,
                ResourceType::OnlineTileSources,
                filePath,
                fileSize,
                fileName);
            pLocalResource->_metadata.reset(new OnlineTileSourcesMetadata(sources));
            std::shared_ptr<const LocalResource> localResource(pLocalResource);
            outResult.insert(resourceId, qMove(localResource));
        }
    }

    return true;
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager_P::LocalResource> > OsmAnd::ResourcesManager_P::getLocalResources() const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    return detachedOf(_localResources);
}

std::shared_ptr<const OsmAnd::ResourcesManager_P::LocalResource> OsmAnd::ResourcesManager_P::getLocalResource(const QString& id) const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    const auto citResource = _localResources.constFind(id);
    if (citResource == _localResources.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::isLocalResource(const QString& id) const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    return _localResources.contains(id);
}

bool OsmAnd::ResourcesManager_P::parseRepository(QXmlStreamReader& xmlReader, QList< std::shared_ptr<const ResourceInRepository> >& repository) const
{
    bool ok = false;
    while(!xmlReader.atEnd() && !xmlReader.hasError())
    {
        xmlReader.readNext();
        if (!xmlReader.isStartElement())
            continue;
        const auto tagName = xmlReader.name();
        const auto& attribs = xmlReader.attributes();

        const auto& resourceTypeValue = attribs.value(QLatin1String("type"));
        if (resourceTypeValue.isNull())
            continue;
        const auto& nameValue = attribs.value(QLatin1String("name"));
        if (nameValue.isNull())
            continue;
        const auto& timestampValue = attribs.value(QLatin1String("timestamp"));
        if (timestampValue.isNull())
            continue;
        const auto& containerSizeValue = attribs.value(QLatin1String("containerSize"));
        if (containerSizeValue.isNull())
            continue;
        const auto& contentSizeValue = attribs.value(QLatin1String("contentSize"));
        if (contentSizeValue.isNull())
            continue;

        const auto name = nameValue.toString();

        auto resourceType = ResourceType::Unknown;
        if (resourceTypeValue == QLatin1String("map"))
            resourceType = ResourceType::MapRegion;
        if (resourceTypeValue == QLatin1String("voice"))
            resourceType = ResourceType::VoicePack;
        if (resourceType == ResourceType::Unknown)
        {
            LogPrintf(LogSeverityLevel::Warning, "Unknown resource type '%s' for '%s'", qPrintableRef(resourceTypeValue), qPrintable(name));
            continue;
        }

        const auto timestamp = timestampValue.toULongLong(&ok);
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Warning, "Invalid timestamp '%s' for '%s'", qPrintableRef(timestampValue), qPrintable(name));
            continue;
        }

        const auto containerSize = containerSizeValue.toULongLong(&ok);
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Warning, "Invalid container size '%s' for '%s'", qPrintableRef(containerSizeValue), qPrintable(name));
            continue;
        }

        const auto contentSize = contentSizeValue.toULongLong(&ok);
        if (!ok)
        {
            LogPrintf(LogSeverityLevel::Warning, "Invalid content size '%s' for '%s'", qPrintableRef(contentSizeValue), qPrintable(name));
            continue;
        }

        QString resourceId;
        QString downloadUrl;
        switch(resourceType)
        {
        case ResourceType::MapRegion:
            // '[region]_2.obf.zip' -> '[region].map.obf'
            resourceId = QString(name)
                .remove(QLatin1String("_2.obf.zip")).toLower()
                .append(QLatin1String(".map.obf"));
            downloadUrl = owner->repositoryBaseUrl + QLatin1String("/download.php?file=") + QUrl::toPercentEncoding(name);
            break;
        case ResourceType::VoicePack:
            // '[language]_0.voice.zip' -> '[resourceName].voice'
            resourceId = QString(name)
                .remove(QLatin1String("_0.voice.zip")).toLower()
                .append(QLatin1String(".voice"));
            downloadUrl = owner->repositoryBaseUrl + QLatin1String("/download.php?file=") + QUrl::toPercentEncoding(name);
            break;
        default:
            /*
            if ($file == "World_basemap_2.obf.zip")
            {
            dwFile('indexes/'.$file, 'standard=yes&file='.$file, "");
            }
            else if (isset($_GET['srtm']))
            {
            dwFile('srtm/'.$file, 'srtm=yes&file='.$file, "srtm");
            }
            else if (isset($_GET['srtmcountry']))
            {
            dwFile('srtm-countries/'.$file, 'srtmcountry=yes&file='.$file, "srtm");
            }
            else if (isset($_GET['road']))
            {
            dwFile('road-indexes/'.$file, 'road=yes&file='.$file, "road");
            }
            else if (isset($_GET['hillshade']))
            {
            dwFile('hillshade/'.$file, 'hillshade=yes&file='.$file, "hillshade");
            }
            else if (isset($_GET['tour']))
            {
            dwFile('tours/'.$file, 'tour=yes&file='.$file, "tour");
            }
            */
            continue;
        }

        std::shared_ptr<const ResourceInRepository> resource(new ResourceInRepository(
            resourceId,
            resourceType,
            downloadUrl,
            contentSize,
            timestamp,
            containerSize));
        repository.push_back(qMove(resource));

#if OSMAND_DEBUG
        {
            QReadLocker scopedLocker(&_localResourcesLock);

            const auto& citLocalResource = _localResources.constFind(resourceId);
            if (citLocalResource != _localResources.cend())
            {
                const auto& localResource = *citLocalResource;
                if (localResource->origin == ResourceOrigin::Installed)
                {
                    const auto& installedResource = std::static_pointer_cast<const InstalledResource>(localResource);
                    if (installedResource->timestamp > timestamp)
                    {
                        LogPrintf(LogSeverityLevel::Warning, "Installed resource '%s' is newer than in repository (%" PRIu64 " > %" PRIu64 ")",
                            qPrintable(resourceId),
                            installedResource->timestamp,
                            timestamp);
                    }
                }
            }
        }
#endif // OSMAND_DEBUG
    }
    if (xmlReader.hasError())
    {
        LogPrintf(LogSeverityLevel::Warning, "XML error: %s (%d, %d)", qPrintable(xmlReader.errorString()), xmlReader.lineNumber(), xmlReader.columnNumber());
        return false;
    }

    return true;
}

void OsmAnd::ResourcesManager_P::loadRepositoryFromCache()
{
    QWriteLocker scopedLocker(&_resourcesInRepositoryLock);

    QList< std::shared_ptr<const ResourceInRepository> > resources;
    QFile repositoryCache(QDir(owner->localStoragePath).absoluteFilePath("repository.cache.xml"));
    bool ok = false;
    if (repositoryCache.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QXmlStreamReader xmlReader(&repositoryCache);
        ok = parseRepository(xmlReader, resources);
    }
    repositoryCache.close();
    if (!ok)
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
    const auto& downloadResult = WebClient().downloadData(QUrl(owner->repositoryBaseUrl + QLatin1String("/get_indexes.php")), &requestResult);
    if (downloadResult.isNull() || !requestResult->isSuccessful())
        return false;

    // Parse XML
    QList< std::shared_ptr<const ResourceInRepository> > resources;
    QXmlStreamReader xmlReader(downloadResult);
    bool ok = parseRepository(xmlReader, resources);
    if (!ok)
        return false;

    // Save repository locally
    QFile repositoryCache(QDir(owner->localStoragePath).absoluteFilePath("repository.cache.xml"));
    if (repositoryCache.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        ok = repositoryCache.write(downloadResult);
        ok = repositoryCache.flush() && ok;
        repositoryCache.close();
        if (!ok)
            repositoryCache.remove();
    }

    // Update repository in memory
    _resourcesInRepository.clear();
    for(auto& entry : resources)
        _resourcesInRepository.insert(entry->id, qMove(entry));
    _resourcesInRepositoryLoaded = true;

    return true;
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager_P::ResourceInRepository> > OsmAnd::ResourcesManager_P::getResourcesInRepository() const
{
    QReadLocker scopedLocker(&_resourcesInRepositoryLock);

    return detachedOf(_resourcesInRepository);
}

std::shared_ptr<const OsmAnd::ResourcesManager_P::ResourceInRepository> OsmAnd::ResourcesManager_P::getResourceInRepository(const QString& id) const
{
    QReadLocker scopedLocker(&_resourcesInRepositoryLock);

    const auto citResource = _resourcesInRepository.constFind(id);
    if (citResource == _resourcesInRepository.cend())
        return nullptr;
    return *citResource;
}

bool OsmAnd::ResourcesManager_P::isResourceInRepository(const QString& id) const
{
    QReadLocker scopedLocker(&_resourcesInRepositoryLock);

    return _resourcesInRepository.contains(id);
}

bool OsmAnd::ResourcesManager_P::isResourceInstalled(const QString& id) const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    const auto citResource = _localResources.constFind(id);
    if (citResource == _localResources.cend())
        return false;

    const auto& resource = *citResource;

    return (resource->origin == ResourceOrigin::Installed);
}

bool OsmAnd::ResourcesManager_P::uninstallResource(const QString& id)
{
    QWriteLocker scopedLocker(&_localResourcesLock);
    bool ok;

    const auto itResource = _localResources.find(id);
    if (itResource == _localResources.end())
        return false;

    const auto resource = *itResource;
    if (resource->origin != ResourceOrigin::Installed)
        return false;
    const auto& installedResource = std::static_pointer_cast<const InstalledResource>(resource);

    // Lock for writing, this lock will never be released
    if (!installedResource->_lock.lockForWriting())
        return false;

    switch(resource->type)
    {
    case ResourceType::MapRegion:
        ok = uninstallMapRegion(installedResource);
        break;
    case ResourceType::VoicePack:
        ok = uninstallVoicePack(installedResource);
        break;
    default:
        return false;
    }
    if (!ok)
        return false;

    _localResources.erase(itResource);

    scopedLocker.unlock();
    owner->localResourcesChangeObservable.postNotify(owner,
        QList<QString>(),
        QList<QString>() << resource->id,
        QList<QString>());

    return true;
}

bool OsmAnd::ResourcesManager_P::uninstallMapRegion(const std::shared_ptr<const InstalledResource>& resource)
{
    return QFile(resource->localPath).remove();
}

bool OsmAnd::ResourcesManager_P::uninstallVoicePack(const std::shared_ptr<const InstalledResource>& resource)
{
    return QDir(resource->localPath).removeRecursively();
}

bool OsmAnd::ResourcesManager_P::installFromFile(const QString& filePath, const ResourceType resourceType)
{
    const auto guessedResourceName = QFileInfo(filePath).fileName().remove(QLatin1String(".zip")).toLower();
    return installFromFile(guessedResourceName, filePath, resourceType);
}

bool OsmAnd::ResourcesManager_P::installFromFile(const QString& id, const QString& filePath, const ResourceType resourceType)
{
    QWriteLocker scopedLocker(&_localResourcesLock);

    const auto itResource = _localResources.find(id);
    if (itResource != _localResources.end())
        return false;

    bool ok = false;
    std::shared_ptr<const InstalledResource> resource;
    switch(resourceType)
    {
    case ResourceType::MapRegion:
        ok = installMapRegionFromFile(id, filePath, resource);
        break;
    case ResourceType::VoicePack:
        ok = installVoicePackFromFile(id, filePath, resource);
        break;
    }

    scopedLocker.unlock();

    if (ok)
    {
        owner->localResourcesChangeObservable.postNotify(owner,
            QList<QString>() << resource->id,
            QList<QString>(),
            QList<QString>());
    }

    return ok;
}

bool OsmAnd::ResourcesManager_P::installMapRegionFromFile(
    const QString& id,
    const QString& filePath,
    std::shared_ptr<const InstalledResource>& outResource,
    const QString localPath_ /*= QString::null*/)
{
    assert(id.endsWith(".map.obf"));

    ArchiveReader archive(filePath);

    // List items
    bool ok = false;
    const auto archiveItems = archive.getItems(&ok);
    if (!ok)
        return false;

    // Find the OBF file
    ArchiveReader::Item obfArchiveItem;
    for(const auto& archiveItem : constOf(archiveItems))
    {
        if (!archiveItem.isValid() || !archiveItem.name.endsWith(QLatin1String(".obf")))
            continue;

        obfArchiveItem = archiveItem;
        break;
    }
    if (!obfArchiveItem.isValid())
        return false;

    // Extract that file without keeping directory structure
    const auto localFileName = localPath_.isNull() ? QDir(owner->localStoragePath).absoluteFilePath(id) : localPath_;
    if (!archive.extractItemToFile(obfArchiveItem.name, localFileName))
        return false;

    // Read information from OBF
    const std::shared_ptr<const ObfFile> obfFile(new ObfFile(localFileName));
    if (!ObfReader(obfFile).obtainInfo())
    {
        LogPrintf(LogSeverityLevel::Warning, "Failed to open OBF '%s'", qPrintable(localFileName));
        QFile(filePath).remove();
        return false;
    }

    // Create local resource entry
    const auto pLocalResource = new InstalledResource(
        id,
        ResourceType::MapRegion,
        filePath,
        obfFile->fileSize,
        obfFile->obfInfo->creationTimestamp);
    outResource.reset(pLocalResource);
    pLocalResource->_metadata.reset(new ObfMetadata(obfFile));
    _localResources.insert(id, outResource);

    return true;
}

bool OsmAnd::ResourcesManager_P::installVoicePackFromFile(
    const QString& id,
    const QString& filePath,
    std::shared_ptr<const InstalledResource>& outResource,
    const QString localPath_ /*= QString::null*/)
{
    assert(id.endsWith(".voice"));

    ArchiveReader archive(filePath);

    // List items
    bool ok = false;
    const auto archiveItems = archive.getItems(&ok);
    if (!ok)
        return false;

    // Verify voice pack
    ArchiveReader::Item voicePackConfigItem;
    for(const auto& archiveItem : constOf(archiveItems))
    {
        if (!archiveItem.isValid() || archiveItem.name != QLatin1String("_config.p"))
            continue;

        voicePackConfigItem = archiveItem;
        break;
    }
    if (!voicePackConfigItem.isValid())
        return false;

    // Extract all files to local directory
    const auto localDirectoryName = localPath_.isNull() ? QDir(owner->localStoragePath).absoluteFilePath(id) : localPath_;
    uint64_t contentSize = 0;
    if (!archive.extractAllItemsTo(localDirectoryName, &contentSize))
        return false;

    // Create special timestamp file
    QFile timestampFile(QDir(localDirectoryName).absoluteFilePath(QLatin1String(".timestamp")));
    timestampFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    QTextStream(&timestampFile) << voicePackConfigItem.modificationTime.toMSecsSinceEpoch();
    timestampFile.flush();
    timestampFile.close();

    // Create special size file
    QFile sizeFile(QDir(localDirectoryName).absoluteFilePath(QLatin1String(".size")));
    sizeFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
    QTextStream(&sizeFile) << contentSize;
    sizeFile.flush();
    sizeFile.close();

    // Create local resource entry
    outResource.reset(new InstalledResource(
        id,
        ResourceType::VoicePack,
        localDirectoryName,
        contentSize,
        voicePackConfigItem.modificationTime.toMSecsSinceEpoch()));
    _localResources.insert(id, outResource);

    return true;
}

bool OsmAnd::ResourcesManager_P::installFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback)
{
    if (isResourceInstalled(id))
        return false;

    const auto& resourceInRepository = getResourceInRepository(id);
    if (!resourceInRepository)
        return false;

    const auto tmpFilePath = QDir(owner->localTemporaryPath).absoluteFilePath(QString("%1.%2")
        .arg(QString(QCryptographicHash::hash(id.toLocal8Bit(), QCryptographicHash::Md5).toHex()))
        .arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));

    bool ok = _webClient.downloadFile(resourceInRepository->url, tmpFilePath, nullptr, downloadProgressCallback);
    if (!ok)
        return false;

    if (!installFromFile(id, tmpFilePath, resourceInRepository->type))
    {
        QFile(tmpFilePath).remove();
        return false;
    }

    QFile(tmpFilePath).remove();
    return true;
}

bool OsmAnd::ResourcesManager_P::installFromRepository(const QString& id, const QString& filePath)
{
    if (isResourceInstalled(id))
        return false;

    const auto& resourceInRepository = getResourceInRepository(id);
    if (!resourceInRepository)
        return false;

    return installFromFile(id, filePath, resourceInRepository->type);
}

bool OsmAnd::ResourcesManager_P::isInstalledResourceOutdated(const QString& id) const
{
    const auto& resourceInRepository = getResourceInRepository(id);
    if (!resourceInRepository)
        return false;
    const auto& localResource = getLocalResource(id);
    if (!localResource || localResource->origin != ResourceOrigin::Installed)
        return false;
    const auto& installedResource = std::static_pointer_cast<const InstalledResource>(localResource);

    const bool outdated = (installedResource->timestamp < resourceInRepository->timestamp);
    if (!outdated && (installedResource->timestamp > resourceInRepository->timestamp))
    {
        LogPrintf(LogSeverityLevel::Warning, "Installed resource '%s' is newer than in repository (%" PRIu64 " > %" PRIu64 ")",
            qPrintable(id),
            installedResource->timestamp,
            resourceInRepository->timestamp);
    }

    return outdated;
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> > OsmAnd::ResourcesManager_P::getOutdatedInstalledResources() const
{
    QReadLocker scopedLocker(&_localResourcesLock);

    QHash< QString, std::shared_ptr<const LocalResource> > resourcesWithUpdates;
    for(const auto& localResource : constOf(_localResources))
    {
        if (localResource->origin != ResourceOrigin::Installed)
            continue;
        const auto& installedResource = std::static_pointer_cast<const InstalledResource>(localResource);
        const auto& resourceInRepository = getResourceInRepository(localResource->id);
        if (!resourceInRepository)
            continue;

        if (installedResource->timestamp >= resourceInRepository->timestamp)
            continue;

        resourcesWithUpdates.insert(localResource->id, localResource);
    }

    return resourcesWithUpdates;
}

bool OsmAnd::ResourcesManager_P::updateFromFile(const QString& filePath)
{
    const auto guessedResourceId = QFileInfo(filePath).fileName().remove(QLatin1String(".zip")).toLower();
    return updateFromFile(guessedResourceId, filePath);
}

bool OsmAnd::ResourcesManager_P::updateMapRegionFromFile(std::shared_ptr<const InstalledResource>& resource, const QString& filePath)
{
    if (!resource->_lock.lockForWriting())
        return false;

    bool ok;
    const auto localPath = resource->localPath;
    ok = uninstallMapRegion(resource);
    ok = installMapRegionFromFile(resource->id, filePath, resource, localPath);

    return ok;
}

bool OsmAnd::ResourcesManager_P::updateVoicePackFromFile(std::shared_ptr<const InstalledResource>& resource, const QString& filePath)
{
    if (!resource->_lock.lockForWriting())
        return false;

    bool ok;
    const auto localPath = resource->localPath;
    ok = uninstallVoicePack(resource);
    ok = installVoicePackFromFile(resource->id, filePath, resource, localPath);

    return ok;
}

bool OsmAnd::ResourcesManager_P::updateFromFile(const QString& id, const QString& filePath)
{
    QWriteLocker scopedLocker(&_localResourcesLock);

    const auto itResource = _localResources.find(id);
    if (itResource == _localResources.end())
        return false;
    const auto& localResource = *itResource;
    if (localResource->origin != ResourceOrigin::Installed)
        return false;
    auto installedResource = std::static_pointer_cast<const InstalledResource>(localResource);

    bool ok = false;
    switch(localResource->type)
    {
    case ResourceType::MapRegion:
        ok = updateMapRegionFromFile(installedResource, filePath);
        break;
    case ResourceType::VoicePack:
        ok = updateVoicePackFromFile(installedResource, filePath);
        break;
    }
    if (!ok)
        return false;

    *itResource = installedResource;

    scopedLocker.unlock();

    owner->localResourcesChangeObservable.postNotify(owner,
        QList<QString>(),
        QList<QString>(),
        QList<QString>() << localResource->id);

    return true;
}

bool OsmAnd::ResourcesManager_P::updateFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback)
{
    const auto& resourceInRepository = getResourceInRepository(id);
    if (!resourceInRepository)
        return false;

    const auto tmpFilePath = QDir(owner->localTemporaryPath).absoluteFilePath(QString("%1.%2")
        .arg(QString(QCryptographicHash::hash(id.toLocal8Bit(), QCryptographicHash::Md5).toHex()))
        .arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch()));

    bool ok = _webClient.downloadFile(resourceInRepository->url, tmpFilePath, nullptr, downloadProgressCallback);
    if (!ok)
        return false;

    ok = updateFromFile(id, tmpFilePath);

    QFile(tmpFilePath).remove();
    return ok;
}

bool OsmAnd::ResourcesManager_P::updateFromRepository(const QString& id, const QString& filePath)
{
    const auto& resourceInRepository = getResourceInRepository(id);
    if (!resourceInRepository)
        return false;

    return updateFromFile(id, filePath);
}

OsmAnd::ResourcesManager_P::OnlineTileSourcesProxy::OnlineTileSourcesProxy(ResourcesManager_P* owner_)
: owner(owner_)
{
}

OsmAnd::ResourcesManager_P::OnlineTileSourcesProxy::~OnlineTileSourcesProxy()
{
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager_P::OnlineTileSourcesProxy::Source> > OsmAnd::ResourcesManager_P::OnlineTileSourcesProxy::getCollection() const
{
    QHash< QString, std::shared_ptr<const OnlineTileSourcesProxy::Source> > result;

    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        if (builtinResource->type != ResourceType::OnlineTileSources)
            continue;

        const auto& sources = std::static_pointer_cast<const OnlineTileSourcesMetadata>(builtinResource->_metadata)->sources;
        result.unite(sources->getCollection());
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::OnlineTileSources)
                continue;

            const auto sources = std::static_pointer_cast<const OnlineTileSourcesMetadata>(localResource->_metadata)->sources;
            result.unite(sources->getCollection());
        }
    }

    return result;
}

std::shared_ptr<const OsmAnd::ResourcesManager_P::OnlineTileSourcesProxy::Source> OsmAnd::ResourcesManager_P::OnlineTileSourcesProxy::getSourceByName(const QString& sourceName) const
{
    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        if (builtinResource->type != ResourceType::OnlineTileSources)
            continue;

        const auto& sources = std::static_pointer_cast<const OnlineTileSourcesMetadata>(builtinResource->_metadata)->sources;
        const auto result = sources->getSourceByName(sourceName);
        if (result)
            return result;
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::OnlineTileSources)
                continue;

            const auto& sources = std::static_pointer_cast<const OnlineTileSourcesMetadata>(localResource->_metadata)->sources;
            const auto result = sources->getSourceByName(sourceName);
            if (result)
                return result;
        }
    }

    return nullptr;
}

OsmAnd::ResourcesManager_P::ObfDataInterfaceProxy::ObfDataInterfaceProxy(
    const QList< std::shared_ptr<const ObfReader> >& obfReaders_,
    const QList< std::shared_ptr<const InstalledResource> >& lockedResources_)
    : ObfDataInterface(obfReaders_)
    , lockedResources(lockedResources_)
{
}

OsmAnd::ResourcesManager_P::ObfDataInterfaceProxy::~ObfDataInterfaceProxy()
{
    for(const auto& lockedResource : constOf(lockedResources))
        lockedResource->_lock.unlockFromReading();
}

OsmAnd::ResourcesManager_P::ObfsCollectionProxy::ObfsCollectionProxy(ResourcesManager_P* owner_)
: owner(owner_)
{
}

OsmAnd::ResourcesManager_P::ObfsCollectionProxy::~ObfsCollectionProxy()
{
}

QVector< std::shared_ptr<const OsmAnd::ObfFile> > OsmAnd::ResourcesManager_P::ObfsCollectionProxy::getObfFiles() const
{
    QReadLocker scopedLocker(&owner->_localResourcesLock);

    bool otherBasemapPresent = false;
    QVector< std::shared_ptr<const ObfFile> > obfFiles;
    for(const auto& localResource : constOf(owner->_localResources))
    {
        if (localResource->type != ResourceType::MapRegion)
            continue;

        const auto& obfMetadata = std::static_pointer_cast<const ObfMetadata>(localResource->_metadata);
        if (obfMetadata->obfFile->obfInfo->isBasemap)
            otherBasemapPresent = true;
        obfFiles.push_back(obfMetadata->obfFile);
    }
    if (!otherBasemapPresent && owner->_miniBasemapObfFile)
        obfFiles.push_back(owner->_miniBasemapObfFile);

    return obfFiles;
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ResourcesManager_P::ObfsCollectionProxy::obtainDataInterface() const
{
    QReadLocker scopedLocker(&owner->_localResourcesLock);

    bool otherBasemapPresent = false;
    QList< std::shared_ptr<const InstalledResource> > lockedResources;
    QList< std::shared_ptr<const ObfReader> > obfReaders;
    for(const auto& localResource : constOf(owner->_localResources))
    {
        if (localResource->type != ResourceType::MapRegion)
            continue;

        const auto& obfMetadata = std::static_pointer_cast<const ObfMetadata>(localResource->_metadata);
        if (!obfMetadata)
            continue;

        if (const auto installedResource = std::dynamic_pointer_cast<const InstalledResource>(localResource))
        {
            if (!installedResource->_lock.tryLockForReading())
                continue;
            lockedResources.push_back(installedResource);
        }

        if (obfMetadata->obfFile->obfInfo->isBasemap)
            otherBasemapPresent = true;
        std::shared_ptr<const ObfReader> obfReader(new ObfReader(obfMetadata->obfFile));
        obfReaders.push_back(qMove(obfReader));
    }
    if (!otherBasemapPresent && owner->_miniBasemapObfFile)
    {
        std::shared_ptr<const ObfReader> obfReader(new ObfReader(owner->_miniBasemapObfFile));
        obfReaders.push_back(qMove(obfReader));
    }

    return std::shared_ptr<ObfDataInterface>(new ObfDataInterfaceProxy(obfReaders, lockedResources));
}

OsmAnd::ResourcesManager_P::MapStylesCollectionProxy::MapStylesCollectionProxy(ResourcesManager_P* owner_)
: owner(owner_)
{
}

OsmAnd::ResourcesManager_P::MapStylesCollectionProxy::~MapStylesCollectionProxy()
{
}

QList< std::shared_ptr<const OsmAnd::MapStyle> > OsmAnd::ResourcesManager_P::MapStylesCollectionProxy::getCollection() const
{
    QList< std::shared_ptr<const MapStyle> > result;

    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        if (builtinResource->type != ResourceType::MapStyle)
            continue;

        const auto& mapStyle = std::static_pointer_cast<const MapStyleMetadata>(builtinResource->_metadata)->mapStyle;
        result.push_back(mapStyle);
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::MapStyle)
                continue;

            const auto& mapStyle = std::static_pointer_cast<const MapStyleMetadata>(localResource->_metadata)->mapStyle;
            result.push_back(mapStyle);
        }
    }

    return result;
}

std::shared_ptr<const OsmAnd::MapStyle> OsmAnd::ResourcesManager_P::MapStylesCollectionProxy::getAsIsStyle(const QString& styleName) const
{
    auto resourceId = styleName;
    if (!resourceId.endsWith(QLatin1String(".render.xml")))
        resourceId.append(QLatin1String(".render.xml"));

    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        // Skip anything that is not a style
        if (builtinResource->type != ResourceType::MapStyle)
            continue;

        // Skip any style that doesn't match by name
        if (builtinResource->id != resourceId)
            continue;

        const auto& mapStyle = std::static_pointer_cast<const MapStyleMetadata>(builtinResource->_metadata)->mapStyle;
        return mapStyle;
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::MapStyle)
                continue;

            if (localResource->id != resourceId)
                continue;

            const auto& mapStyle = std::static_pointer_cast<const MapStyleMetadata>(localResource->_metadata)->mapStyle;
            return mapStyle;
        }
    }

    return nullptr;
}

bool OsmAnd::ResourcesManager_P::MapStylesCollectionProxy::obtainBakedStyle(const QString& styleName, std::shared_ptr<const MapStyle>& outStyle) const
{
    auto resourceId = styleName.toLower();
    if (!resourceId.endsWith(QLatin1String(".render.xml")))
        resourceId.append(QLatin1String(".render.xml"));

    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        if (builtinResource->type != ResourceType::MapStyle)
            continue;

        if (builtinResource->id != resourceId)
            continue;

        const auto& mapStyle = std::static_pointer_cast<const MapStyleMetadata>(builtinResource->_metadata)->mapStyle;

        outStyle = mapStyle;
        assert(outStyle->isMetadataLoaded() && outStyle->isLoaded());

        return true;
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::MapStyle)
                continue;

            if (localResource->id != resourceId)
                continue;

            const auto& mapStyle = std::static_pointer_cast<const MapStyleMetadata>(localResource->_metadata)->mapStyle;
            if (!mapStyle->load())
                return false;

            outStyle = mapStyle;

            return true;
        }
    }

    return false;
}

OsmAnd::ResourcesManager_P::MapStylesPresetsCollectionProxy::MapStylesPresetsCollectionProxy(ResourcesManager_P* owner_)
: owner(owner_)
{
}

OsmAnd::ResourcesManager_P::MapStylesPresetsCollectionProxy::~MapStylesPresetsCollectionProxy()
{
}

QList< std::shared_ptr<const OsmAnd::MapStylePreset> > OsmAnd::ResourcesManager_P::MapStylesPresetsCollectionProxy::getCollection() const
{
    QList< std::shared_ptr<const MapStylePreset> > result;

    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        if (builtinResource->type != ResourceType::MapStylesPresets)
            continue;

        const auto& presets = std::static_pointer_cast<const MapStylesPresetsMetadata>(builtinResource->_metadata)->presets;
        result << presets->getCollection();
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::MapStylesPresets)
                continue;

            const auto& presets = std::static_pointer_cast<const MapStylesPresetsMetadata>(localResource->_metadata)->presets;
            result << presets->getCollection();
        }
    }

    return result;
}

QList< std::shared_ptr<const OsmAnd::MapStylePreset> > OsmAnd::ResourcesManager_P::MapStylesPresetsCollectionProxy::getCollectionFor(const QString& styleName) const
{
    QList< std::shared_ptr<const MapStylePreset> > result;

    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        if (builtinResource->type != ResourceType::MapStylesPresets)
            continue;

        const auto& presets = std::static_pointer_cast<const MapStylesPresetsMetadata>(builtinResource->_metadata)->presets;
        result << presets->getCollectionFor(styleName);
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::MapStylesPresets)
                continue;

            const auto& presets = std::static_pointer_cast<const MapStylesPresetsMetadata>(localResource->_metadata)->presets;
            result << presets->getCollectionFor(styleName);
        }
    }

    return result;
}

std::shared_ptr<const OsmAnd::MapStylePreset> OsmAnd::ResourcesManager_P::MapStylesPresetsCollectionProxy::getPreset(const QString& styleName, const QString& presetName) const
{
    for(const auto& builtinResource : constOf(owner->_builtinResources))
    {
        if (builtinResource->type != ResourceType::MapStylesPresets)
            continue;

        const auto& presets = std::static_pointer_cast<const MapStylesPresetsMetadata>(builtinResource->_metadata)->presets;
        const auto preset = presets->getPreset(styleName, presetName);
        if (preset)
            return preset;
    }

    {
        QReadLocker scopedLocker(&owner->_localResourcesLock);

        for(const auto& localResource : constOf(owner->_localResources))
        {
            if (localResource->type != ResourceType::MapStyle)
                continue;

            const auto& presets = std::static_pointer_cast<const MapStylesPresetsMetadata>(localResource->_metadata)->presets;
            const auto preset = presets->getPreset(styleName, presetName);
            if (preset)
                return preset;
        }
    }

    return nullptr;
}
