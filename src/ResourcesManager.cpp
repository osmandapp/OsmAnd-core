#include "ResourcesManager.h"
#include "ResourcesManager_P.h"

#include <QStandardPaths>
#include <QDir>

#include "ObfInfo.h"
#include "WeatherTileResourcesManager.h"

OsmAnd::ResourcesManager::ResourcesManager(
    const QString& localStoragePath_,
    const QString& userStoragePath_ /*= QString::null*/,
    const QList<QString>& readonlyExternalStoragePaths_ /*= QList<QString>()*/,
    const QString& miniBasemapFilename_ /*= QString::null*/,
    const QString& localTemporaryPath_ /*= QString::null*/,
    const QString& localCachePath_ /*= QString::null*/,
    const QString& appVersion_ /*= QString::null*/,
    const QString& repositoryBaseUrl_ /*= QLatin1String("http://download.osmand.net")*/,
    const QString& indexesUrl_ /*= QLatin1String("http://download.osmand.net/get_indexes")*/,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
    : _p(new ResourcesManager_P(this, webClient))
    , localStoragePath(localStoragePath_)
    , userStoragePath(userStoragePath_)
    , readonlyExternalStoragePaths(readonlyExternalStoragePaths_)
    , miniBasemapFilename(miniBasemapFilename_)
    , localTemporaryPath(!localTemporaryPath_.isNull()
        ? localTemporaryPath_
        : QStandardPaths::writableLocation(QStandardPaths::TempLocation))
    , repositoryBaseUrl(repositoryBaseUrl_)
    , indexesUrl(indexesUrl_)
    , localCachePath(!localCachePath_.isNull()
        ? localCachePath_
        : QStandardPaths::writableLocation(QStandardPaths::CacheLocation))
    , appVersion(appVersion_)
    , onlineTileSources(_p->onlineTileSources)
    , mapStylesCollection(_p->mapStylesCollection)
    , obfsCollection(_p->obfsCollection)
    , changesManager(_p->changesManager)
{
    QDir(localStoragePath).mkpath(QLatin1String("."));
    QDir(userStoragePath).mkpath(QLatin1String("."));
    QDir(localTemporaryPath).mkpath(QLatin1String("."));

    _p->initialize();
    _p->inflateBuiltInResources();
    _p->scanManagedStoragePath();
    _p->rescanUnmanagedStoragePaths();
    _p->loadRepositoryFromCache();
    _p->attachToFileSystem();
}

OsmAnd::ResourcesManager::~ResourcesManager()
{
    _p->detachFromFileSystem();
}

std::shared_ptr<const OsmAnd::ResourcesManager::Resource> OsmAnd::ResourcesManager::getResource(const QString& id) const
{
    return _p->getResource(id);
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager::BuiltinResource> >
OsmAnd::ResourcesManager::getBuiltInResources() const
{
    return _p->getBuiltInResources();
}

std::shared_ptr<const OsmAnd::ResourcesManager::BuiltinResource>
OsmAnd::ResourcesManager::getBuiltInResource(const QString& id) const
{
    return _p->getBuiltInResource(id);
}

bool OsmAnd::ResourcesManager::isBuiltInResource(const QString& id) const
{
    return _p->isBuiltInResource(id);
}

bool OsmAnd::ResourcesManager::rescanUnmanagedStoragePaths() const
{
    return _p->rescanUnmanagedStoragePaths();
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager_P::LocalResource> > OsmAnd::ResourcesManager::getSortedLocalResources() const
{
    return _p->getSortedLocalResources();
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> > OsmAnd::ResourcesManager::getLocalResources() const
{
    return _p->getLocalResources();
}

std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> OsmAnd::ResourcesManager::getLocalResource(
    const QString& id) const
{
    return _p->getLocalResource(id);
}

bool OsmAnd::ResourcesManager::isLocalResource(const QString& id) const
{
    return _p->isLocalResource(id);
}

bool OsmAnd::ResourcesManager::isRepositoryAvailable() const
{
    return _p->isRepositoryAvailable();
}

bool OsmAnd::ResourcesManager::updateRepository() const
{
    return _p->updateRepository();
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager::ResourceInRepository> >
OsmAnd::ResourcesManager::getResourcesInRepository() const
{
    return _p->getResourcesInRepository();
}

std::shared_ptr<const OsmAnd::ResourcesManager::ResourceInRepository> OsmAnd::ResourcesManager::getResourceInRepository(
    const QString& id) const
{
    return _p->getResourceInRepository(id);
}

bool OsmAnd::ResourcesManager::isResourceInRepository(const QString& id) const
{
    return _p->isResourceInRepository(id);
}

bool OsmAnd::ResourcesManager::isResourceInstalled(const QString& id) const
{
    return _p->isResourceInstalled(id);
}

uint64_t OsmAnd::ResourcesManager::getResourceTimestamp(const QString& id) const
{
    return _p->getResourceTimestamp(id);
}

bool OsmAnd::ResourcesManager::uninstallResource(const QString& id)
{
    return _p->uninstallResource(id);
}

bool OsmAnd::ResourcesManager::uninstallTilesResource(const QString& name)
{
    return _p->uninstallTilesResource(name);
}

bool OsmAnd::ResourcesManager::installTilesResource(const std::shared_ptr<const IOnlineTileSources::Source>& source)
{
    return _p->installTilesResource(source);
}

void OsmAnd::ResourcesManager::installBuiltInTileSources()
{
    _p->installBuiltInTileSources();
}

bool OsmAnd::ResourcesManager::uninstallResource(const std::shared_ptr<const InstalledResource> &installedResource, const std::shared_ptr<const LocalResource> &resource)
{
    return _p->uninstallResource(installedResource, resource);
}

bool OsmAnd::ResourcesManager::addLocalResource(const QString& filePath)
{
    return _p->addLocalResource(filePath);
}

bool OsmAnd::ResourcesManager::installImportedResource(const QString& filePath, const QString& newName, const OsmAnd::ResourcesManager::ResourceType resourceType)
{
    return _p->installImportedResource(filePath, newName, resourceType);
}

bool OsmAnd::ResourcesManager::installFromFile(const QString& filePath, const ResourceType resourceType)
{
    return _p->installFromFile(filePath, resourceType);
}

bool OsmAnd::ResourcesManager::installFromFile(const QString& id, const QString& filePath, const ResourceType resourceType)
{
    return _p->installFromFile(id, filePath, resourceType);
}

bool OsmAnd::ResourcesManager::installFromRepository(
    const QString& id,
    const WebClient::RequestProgressCallbackSignature downloadProgressCallback /*= nullptr*/)
{
    return _p->installFromRepository(id, downloadProgressCallback);
}

bool OsmAnd::ResourcesManager::installFromRepository(const QString& id, const QString& filePath)
{
    return _p->installFromRepository(id, filePath);
}

bool OsmAnd::ResourcesManager::isInstalledResourceOutdated(const QString& id) const
{
    return _p->isInstalledResourceOutdated(id);
}

QHash< QString, std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> >
OsmAnd::ResourcesManager::getOutdatedInstalledResources() const
{
    return _p->getOutdatedInstalledResources();
}

bool OsmAnd::ResourcesManager::updateFromFile(const QString& filePath)
{
    return _p->updateFromFile(filePath);
}

bool OsmAnd::ResourcesManager::updateFromFile(const QString& id, const QString& filePath)
{
    return _p->updateFromFile(id, filePath);
}

bool OsmAnd::ResourcesManager::updateFromRepository(
    const QString& id,
    const WebClient::RequestProgressCallbackSignature downloadProgressCallback /*= nullptr*/)
{
    return _p->updateFromRepository(id, downloadProgressCallback);
}

bool OsmAnd::ResourcesManager::updateFromRepository(const QString& id, const QString& filePath)
{
    return _p->updateFromRepository(id, filePath);
}

const std::shared_ptr<const OsmAnd::OnlineTileSources> OsmAnd::ResourcesManager::downloadOnlineTileSources() const
{
    return _p->downloadOnlineTileSources();
}

OsmAnd::ResourcesManager::ResourceType OsmAnd::ResourcesManager::getIndexType(const QStringRef &resourceTypeValue)
{
    return ResourcesManager_P::getIndexType(resourceTypeValue);
}

const std::shared_ptr<OsmAnd::WeatherTileResourcesManager> OsmAnd::ResourcesManager::getWeatherResourcesManager() const
{
    return _weatherResourcesManager;
}

void OsmAnd::ResourcesManager::instantiateWeatherResourcesManager(
    const QHash<BandIndex, std::shared_ptr<const GeoBandSettings>>& bandSettings,
    const QString& localCachePath,
    const QString& projResourcesPath,
    const uint32_t tileSize /*= 256*/,
    const float densityFactor /*= 1.0f*/,
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/)
{
    _weatherResourcesManager = std::make_shared<WeatherTileResourcesManager>(
        bandSettings,
        localCachePath,
        projResourcesPath,
        tileSize,
        densityFactor,
        webClient
    );
}


OsmAnd::ResourcesManager::Resource::Metadata::Metadata()
{
}

OsmAnd::ResourcesManager::Resource::Metadata::~Metadata()
{
}

OsmAnd::ResourcesManager::Resource::Resource(
    const QString& id_,
    const ResourceType type_,
    const ResourceOrigin origin_)
    : id(id_)
    , type(type_)
    , origin(origin_)
    , metadata(_metadata)
{
    assert(id == id.toLower());
}

OsmAnd::ResourcesManager::Resource::~Resource()
{
}

OsmAnd::ResourcesManager::LocalResource::LocalResource(
    const QString& id_,
    const ResourceType type_,
    const ResourceOrigin origin_,
    const QString& localPath_,
    const uint64_t size_)
    : Resource(id_, type_, origin_)
    , localPath(localPath_)
    , size(size_)
{
    assert(origin_ == ResourceOrigin::Installed || origin_ == ResourceOrigin::Unmanaged);
}

OsmAnd::ResourcesManager::LocalResource::~LocalResource()
{
}

OsmAnd::ResourcesManager::UnmanagedResource::UnmanagedResource(
    const QString& id_,
    const ResourceType type_,
    const QString& localPath_,
    const uint64_t size_,
    const QString& name_)
    : LocalResource(id_, type_, ResourceOrigin::Unmanaged, localPath_, size_)
    , name(name_)
{
}

OsmAnd::ResourcesManager::UnmanagedResource::~UnmanagedResource()
{
}

OsmAnd::ResourcesManager::InstalledResource::InstalledResource(
    const QString& id_,
    const ResourceType type_,
    const QString& localPath_,
    const uint64_t size_,
    const uint64_t timestamp_)
    : LocalResource(id_, type_, ResourceOrigin::Installed, localPath_, size_)
    , timestamp(timestamp_)
{
}

OsmAnd::ResourcesManager::InstalledResource::~InstalledResource()
{
}

OsmAnd::ResourcesManager::BuiltinResource::BuiltinResource(
    const QString& id_,
    const ResourceType type_,
    const std::shared_ptr<const Metadata>& metadata_ /*= nullptr*/)
    : Resource(id_, type_, ResourceOrigin::Builtin)
{
    _metadata = metadata_;
}

OsmAnd::ResourcesManager::BuiltinResource::~BuiltinResource()
{
}

OsmAnd::ResourcesManager::ResourceInRepository::ResourceInRepository(
    const QString& id_,
    const ResourceType type_,
    const QUrl& url_,
    const uint64_t size_,
    const uint64_t timestamp_,
    const uint64_t packageSize_)
    : Resource(id_, type_, ResourceOrigin::Repository)
    , url(url_)
    , size(size_)
    , timestamp(timestamp_)
    , packageSize(packageSize_)
{
}

OsmAnd::ResourcesManager::ResourceInRepository::~ResourceInRepository()
{
}

OsmAnd::ResourcesManager::ObfMetadata::ObfMetadata(const std::shared_ptr<const ObfFile>& obfFile_)
    : obfFile(obfFile_)
{
}

OsmAnd::ResourcesManager::ObfMetadata::~ObfMetadata()
{
}

OsmAnd::ResourcesManager::MapStyleMetadata::MapStyleMetadata(const std::shared_ptr<UnresolvedMapStyle>& mapStyle_)
    : mapStyle(mapStyle_)
{
}

OsmAnd::ResourcesManager::MapStyleMetadata::~MapStyleMetadata()
{
}

OsmAnd::ResourcesManager::OnlineTileSourcesMetadata::OnlineTileSourcesMetadata(
    const std::shared_ptr<const OnlineTileSources>& sources_)
    : sources(sources_)
{
}

OsmAnd::ResourcesManager::OnlineTileSourcesMetadata::~OnlineTileSourcesMetadata()
{
}
