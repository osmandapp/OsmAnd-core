#ifndef _OSMAND_CORE_RESOURCES_MANAGER_P_H_
#define _OSMAND_CORE_RESOURCES_MANAGER_P_H_

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
#include "WebClient.h"
#include "ResourcesManager.h"
#include "ObfDataInterface.h"
#include "IObfsCollection.h"

namespace OsmAnd
{
    class ResourcesManager_P
    {
    public:
        typedef ResourcesManager::ResourceType ResourceType;
        typedef ResourcesManager::ResourceOrigin ResourceOrigin;
        typedef ResourcesManager::Resource Resource;
        typedef ResourcesManager::LocalResource LocalResource;
        typedef ResourcesManager::InstalledResource InstalledResource;
        typedef ResourcesManager::UnmanagedResource UnmanagedResource;
        typedef ResourcesManager::BuiltinResource BuiltinResource;
        typedef ResourcesManager::BuiltinMapStylesPresetsResource BuiltinMapStylesPresetsResource;
        typedef ResourcesManager::BuiltinOnlineTileSourcesResource BuiltinOnlineTileSourcesResource;
        typedef ResourcesManager::ResourceInRepository ResourceInRepository;

    private:
        QFileSystemWatcher* const _fileSystemWatcher;
        QMetaObject::Connection _onDirectoryChangedConnection;
        void onDirectoryChanged(const QString& path);
        QMetaObject::Connection _onFileChangedConnection;
        void onFileChanged(const QString& path);

        void attachToFileSystem();
        void detachFromFileSystem();

        QHash< QString, std::shared_ptr<const BuiltinResource> > _builtinResources;
        void inflateBuiltInResources();

        mutable QReadWriteLock _localResourcesLock;
        mutable QHash< QString, std::shared_ptr<const LocalResource> > _localResources;
        static bool rescanLocalStoragePath(const QString& storagePath, const bool isUnmanagedStorage, QHash< QString, std::shared_ptr<const LocalResource> >& outResult);

        std::shared_ptr<const ObfFile> _miniBasemapObfFile;

        mutable QReadWriteLock _resourcesInRepositoryLock;
        mutable QHash< QString, std::shared_ptr<const ResourceInRepository> > _resourcesInRepository;
        mutable bool _resourcesInRepositoryLoaded;
        bool parseRepository(QXmlStreamReader& xmlReader, QList< std::shared_ptr<const ResourceInRepository> >& repository) const;

        mutable WebClient _webClient;

        bool uninstallMapRegion(const std::shared_ptr<const InstalledResource>& resource);
        bool uninstallVoicePack(const std::shared_ptr<const InstalledResource>& resource);

        bool installMapRegionFromFile(const QString& id, const QString& filePath, std::shared_ptr<const InstalledResource>& outResource);
        bool installVoicePackFromFile(const QString& id, const QString& filePath, std::shared_ptr<const InstalledResource>& outResource);

        bool updateMapRegionFromFile(std::shared_ptr<const InstalledResource>& resource, const QString& filePath);
        bool updateVoicePackFromFile(std::shared_ptr<const InstalledResource>& resource, const QString& filePath);

        struct ObfMetadata : public Resource::Metadata
        {
            ObfMetadata(const std::shared_ptr<const ObfFile>& obfFile_)
                : obfFile(obfFile_)
            {
            }

            virtual ~ObfMetadata()
            {
            }

            const std::shared_ptr<const ObfFile> obfFile;
        };

        class ManagedObfDataInterface : public ObfDataInterface
        {
        private:
        protected:
            ManagedObfDataInterface(
                const QList< std::shared_ptr<const ObfReader> >& obfReaders,
                const QList< std::shared_ptr<const InstalledResource> >& lockedResources);
        public:
            virtual ~ManagedObfDataInterface();

            const QList< std::shared_ptr<const InstalledResource> > lockedResources;

        friend class OsmAnd::ResourcesManager_P;
        };

        class ObfsCollection : public IObfsCollection
        {
        private:
        protected:
            ObfsCollection(ResourcesManager_P* owner);
        public:
            virtual ~ObfsCollection();

            ResourcesManager_P* const owner;

            virtual QVector< std::shared_ptr<const ObfFile> > getObfFiles() const;
            virtual std::shared_ptr<ObfDataInterface> obtainDataInterface() const;

        friend class OsmAnd::ResourcesManager_P;
        };
    protected:
        ResourcesManager_P(ResourcesManager* owner);

        void initialize();
        void loadRepositoryFromCache();
        bool scanManagedStoragePath();
    public:
        virtual ~ResourcesManager_P();

        ImplementationInterface<ResourcesManager> owner;

        // Built-in resources:
        QList< std::shared_ptr<const BuiltinResource> > getBuiltInResources() const;
        std::shared_ptr<const BuiltinResource> getBuiltInResource(const QString& id) const;
        bool isBuiltInResource(const QString& id) const;

        // Local resources:
        bool rescanUnmanagedStoragePaths() const;
        QList< std::shared_ptr<const LocalResource> > getLocalResources() const;
        std::shared_ptr<const LocalResource> getLocalResource(const QString& id) const;
        bool isLocalResource(const QString& id) const;

        // Resources in repository:
        bool isRepositoryAvailable() const;
        bool updateRepository() const;
        QList< std::shared_ptr<const ResourceInRepository> > getResourcesInRepository() const;
        std::shared_ptr<const ResourceInRepository> getResourceInRepository(const QString& id) const;
        bool isResourceInRepository(const QString& id) const;

        // Install / Uninstall:
        bool isResourceInstalled(const QString& id) const;
        bool uninstallResource(const QString& id);
        bool installFromFile(const QString& filePath, const ResourceType resourceType);
        bool installFromFile(const QString& id, const QString& filePath, const ResourceType resourceType);
        bool installFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback);

        // Updates:
        bool isInstalledResourceOutdated(const QString& id) const;
        QList<QString> getOutdatedInstalledResources() const;
        bool updateFromFile(const QString& filePath);
        bool updateFromFile(const QString& id, const QString& filePath);
        bool updateFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback);

        // OBFs collection
        const std::shared_ptr<const IObfsCollection> obfsCollection;

    friend class OsmAnd::ResourcesManager;
    friend class OsmAnd::ResourcesManager_P::ObfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_P_H_)
