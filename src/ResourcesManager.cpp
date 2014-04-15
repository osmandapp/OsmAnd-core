#include "ResourcesManager.h"
#include "ResourcesManager_P.h"

#include <QStandardPaths>

#include "ObfInfo.h"

OsmAnd::ResourcesManager::ResourcesManager(
    const QString& localStoragePath_,
    const QList<QString>& extraStoragePaths_ /*= QList<QString>()*/,
    const QString& miniBasemapFilename_ /*= QString::null*/,
    const QString& localTemporaryPath_ /*= QString::null*/,
    const QString& repositoryBaseUrl_ /*= QLatin1String("http://download.osmand.net")*/)
    : _p(new ResourcesManager_P(this))
    , localStoragePath(localStoragePath_)
    , extraStoragePaths(extraStoragePaths_)
    , miniBasemapFilename(miniBasemapFilename_)
    , localTemporaryPath(!localTemporaryPath_.isNull() ? localTemporaryPath_ : QStandardPaths::writableLocation(QStandardPaths::TempLocation))
    , repositoryBaseUrl(repositoryBaseUrl_)
{
    _p->refreshRepositoryIndex();
    _p->attachToFileSystem();
}

OsmAnd::ResourcesManager::~ResourcesManager()
{
    _p->detachFromFileSystem();
}

bool OsmAnd::ResourcesManager::rescanLocalStoragePaths() const
{
    return _p->rescanLocalStoragePaths();
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> > OsmAnd::ResourcesManager::getLocalResources() const
{
    return _p->getLocalResources();
}

std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> OsmAnd::ResourcesManager::getLocalResource(const QString& name) const
{
    return _p->getLocalResource(name);
}

bool OsmAnd::ResourcesManager::refreshRepositoryIndex() const
{
    return _p->refreshRepositoryIndex();
}

QList< std::shared_ptr<const OsmAnd::ResourcesManager::ResourceInRepository> > OsmAnd::ResourcesManager::getRepositoryIndex() const
{
    return _p->getRepositoryIndex();
}

std::shared_ptr<const OsmAnd::ResourcesManager::ResourceInRepository> OsmAnd::ResourcesManager::getResourceInRepository(const QString& name) const
{
    return _p->getResourceInRepository(name);
}

bool OsmAnd::ResourcesManager::isResourceInstalled(const QString& name) const
{
    return _p->isResourceInstalled(name);
}

bool OsmAnd::ResourcesManager::uninstallResource(const QString& name)
{
    return _p->uninstallResource(name);
}

bool OsmAnd::ResourcesManager::installFromFile(const QString& filePath, const ResourceType resourceType)
{
    return _p->installFromFile(filePath, resourceType);
}

bool OsmAnd::ResourcesManager::installFromFile(const QString& name, const QString& filePath, const ResourceType resourceType)
{
    return _p->installFromFile(name, filePath, resourceType);
}

bool OsmAnd::ResourcesManager::installFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback /*= nullptr*/)
{
    return _p->installFromRepository(name, downloadProgressCallback);
}

std::shared_ptr<const OsmAnd::IObfsCollection> OsmAnd::ResourcesManager::getObfsCollection() const
{
    return _p->getObfsCollection();
}

bool OsmAnd::ResourcesManager::updateAvailableInRepositoryFor(const QString& name) const
{
    return _p->updateAvailableInRepositoryFor(name);
}

QList<QString> OsmAnd::ResourcesManager::getAvailableUpdatesFromRepository() const
{
    return _p->getAvailableUpdatesFromRepository();
}

bool OsmAnd::ResourcesManager::updateFromFile(const QString& filePath)
{
    return _p->updateFromFile(filePath);
}

bool OsmAnd::ResourcesManager::updateFromFile(const QString& name, const QString& filePath)
{
    return _p->updateFromFile(name, filePath);
}

bool OsmAnd::ResourcesManager::updateFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback /*= nullptr*/)
{
    return _p->updateFromRepository(name, downloadProgressCallback);
}

OsmAnd::ResourcesManager::Resource::Resource(const QString& name_, const ResourceType type_, const uint64_t timestamp_, const uint64_t contentSize_)
    : name(name_)
    , type(type_)
    , timestamp(timestamp_)
    , contentSize(contentSize_)
{
}

OsmAnd::ResourcesManager::Resource::~Resource()
{
}

OsmAnd::ResourcesManager::LocalResource::LocalResource(
    const QString& name_,
    const ResourceType type_,
    const uint64_t timestamp_,
    const uint64_t contentSize_,
    const QString& localPath_)
    : Resource(name_, type_, timestamp_, contentSize_)
    , localPath(localPath_)
{
}

OsmAnd::ResourcesManager::LocalResource::~LocalResource()
{
}

OsmAnd::ResourcesManager::LocalObfResource::LocalObfResource(
    const QString& name_,
    const ResourceType type_,
    const std::shared_ptr<const ObfFile>& obfFile_)
    : LocalResource(name_, type_, obfFile_->obfInfo->creationTimestamp, obfFile_->fileSize, obfFile_->filePath)
    , obfInfo(obfFile_->obfInfo)
{
}

OsmAnd::ResourcesManager::LocalObfResource::LocalObfResource(
    const QString& name_,
    const ResourceType type_,
    const uint64_t contentSize_,
    const QString& localPath_,
    const std::shared_ptr<const ObfInfo>& obfInfo_)
    : LocalResource(name_, type_, obfInfo_->creationTimestamp, contentSize_, localPath_)
    , obfInfo(obfInfo_)
{
}

OsmAnd::ResourcesManager::LocalObfResource::~LocalObfResource()
{
}

OsmAnd::ResourcesManager::ResourceInRepository::ResourceInRepository(
    const QString& name_,
    const ResourceType type_,
    const uint64_t timestamp_,
    const uint64_t contentSize_,
    const QString& containerDownloadUrl_,
    const uint64_t containerSize_)
    : Resource(name_, type_, timestamp_, contentSize_)
    , containerDownloadUrl(containerDownloadUrl_)
    , containerSize(containerSize_)
{
}

OsmAnd::ResourcesManager::ResourceInRepository::~ResourceInRepository()
{
}
