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

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "WebClient.h"
#include "ResourcesManager.h"
#include "IObfsCollection.h"

namespace OsmAnd
{
    class ResourcesManager_P
    {
    public:
        typedef ResourcesManager::ResourceType ResourceType;
        typedef ResourcesManager::LocalResource LocalResource;
        typedef ResourcesManager::LocalObfResource LocalObfResource;
        typedef ResourcesManager::ResourceInRepository ResourceInRepository;

    private:
        QFileSystemWatcher* const _fileSystemWatcher;
        QMetaObject::Connection _onDirectoryChangedConnection;
        void onDirectoryChanged(const QString& path);
        QMetaObject::Connection _onFileChangedConnection;
        void onFileChanged(const QString& path);

        void attachToFileSystem();
        void detachFromFileSystem();

        mutable QReadWriteLock _localResourcesLock;
        mutable QHash< QString, std::shared_ptr<const LocalResource> > _localResources;
        static bool rescanLocalStoragePath(const QString& storagePath, QHash< QString, std::shared_ptr<const LocalResource> >& outResult);

        std::shared_ptr<const ObfFile> _miniBasemapObfFile;

        mutable QReadWriteLock _repositoryIndexLock;
        mutable QHash< QString, std::shared_ptr<const ResourceInRepository> > _repositoryIndex;

        mutable WebClient _webClient;

        bool uninstallMapRegion(const std::shared_ptr<const LocalResource>& localResource);
        bool uninstallVoicePack(const std::shared_ptr<const LocalResource>& localResource);

        bool installMapRegionFromFile(const QString& name, const QString& filePath);
        bool installVoicePackFromFile(const QString& name, const QString& filePath);

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
    public:
        virtual ~ResourcesManager_P();

        ImplementationInterface<ResourcesManager> owner;

        // Local installed resources:
        bool rescanLocalStoragePaths() const;
        QList< std::shared_ptr<const LocalResource> > getLocalResources() const;
        std::shared_ptr<const LocalResource> getLocalResource(const QString& name) const;

        // Remote resources in repository:
        bool refreshRepositoryIndex() const;
        QList< std::shared_ptr<const ResourceInRepository> > getRepositoryIndex() const;
        std::shared_ptr<const ResourceInRepository> getResourceInRepository(const QString& name) const;

        // Install / Uninstall:
        bool isResourceInstalled(const QString& name) const;
        bool uninstallResource(const QString& name);
        bool installFromFile(const QString& filePath, const ResourceType resourceType);
        bool installFromFile(const QString& name, const QString& filePath, const ResourceType resourceType);
        bool installFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback);

        // Updates:
        bool updateAvailableInRepositoryFor(const QString& name) const;
        QList<QString> getAvailableUpdatesFromRepository() const;
        bool updateFromFile(const QString& filePath);
        bool updateFromFile(const QString& name, const QString& filePath);
        bool updateFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback);

        // OBFs collection
        const std::shared_ptr<const IObfsCollection> obfsCollection;

    friend class OsmAnd::ResourcesManager;
    friend class OsmAnd::ResourcesManager_P::ObfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_P_H_)
