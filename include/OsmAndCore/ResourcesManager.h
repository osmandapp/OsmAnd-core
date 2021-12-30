#ifndef _OSMAND_CORE_RESOURCES_MANAGER_H_
#define _OSMAND_CORE_RESOURCES_MANAGER_H_

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
#include <OsmAndCore/Map/IOnlineTileSources.h>

namespace OsmAnd
{
    class IOnlineTileSources;
    class IMapStylesCollection;
    class IObfsCollection;
    class UnresolvedMapStyle;
    class OnlineTileSources;
    class IncrementalChangesManager;
    class OnlineTileSources;

    class ResourcesManager_P;
    class OSMAND_CORE_API ResourcesManager
    {
        Q_DISABLE_COPY_AND_MOVE(ResourcesManager);
    public:
        enum class ResourceType
        {
            Unknown = -1,

            MapRegion,
            RoadMapRegion,
            SrtmMapRegion,
            DepthContourRegion,
            WikiMapRegion,
            HillshadeRegion,
            SlopeRegion,
            HeightmapRegion,
            LiveUpdateRegion,
            VoicePack,
            MapStyle,
            MapStylesPresets,
            OnlineTileSources,
            GpxFile,
            SqliteFile
        };

        enum class ResourceOrigin
        {
            Builtin,
            Installed,
            Repository,
            Unmanaged
        };

        class OSMAND_CORE_API Resource
        {
            Q_DISABLE_COPY_AND_MOVE(Resource);

        public:
            struct OSMAND_CORE_API Metadata
            {
                Metadata();
                virtual ~Metadata();
            };

        private:
        protected:
            Resource(
                const QString& id,
                const ResourceType type,
                const ResourceOrigin origin);

            std::shared_ptr<const Metadata> _metadata;
        public:
            virtual ~Resource();

            const QString id;
            const ResourceType type;
            const ResourceOrigin origin;

            const std::shared_ptr<const Metadata>& metadata;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API LocalResource : public Resource
        {
            Q_DISABLE_COPY_AND_MOVE(LocalResource);
        private:
        protected:
            LocalResource(
                const QString& id,
                const ResourceType type,
                const ResourceOrigin origin,
                const QString& localPath,
                const uint64_t size);
        public:
            virtual ~LocalResource();

            const QString localPath;
            const uint64_t size;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API UnmanagedResource : public LocalResource
        {
            Q_DISABLE_COPY_AND_MOVE(UnmanagedResource);
        private:
        protected:
            UnmanagedResource(
                const QString& id,
                const ResourceType type,
                const QString& localPath,
                const uint64_t size,
                const QString& name);
        public:
            virtual ~UnmanagedResource();

            const QString name;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API InstalledResource : public LocalResource
        {
            Q_DISABLE_COPY_AND_MOVE(InstalledResource);
        private:
        protected:
            InstalledResource(
                const QString& id,
                const ResourceType type,
                const QString& localPath,
                const uint64_t size,
                const uint64_t timestamp);

            const AccessLockCounter _lock;
        public:
            virtual ~InstalledResource();

            const uint64_t timestamp;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API BuiltinResource : public Resource
        {
            Q_DISABLE_COPY_AND_MOVE(BuiltinResource);
        private:
        protected:
            BuiltinResource(
                const QString& id,
                const ResourceType type,
                const std::shared_ptr<const Metadata>& metadata = nullptr);
        public:
            virtual ~BuiltinResource();

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API ResourceInRepository : public Resource
        {
            Q_DISABLE_COPY_AND_MOVE(ResourceInRepository);
        private:
        protected:
            ResourceInRepository(
                const QString& id,
                const ResourceType type,
                const QUrl& url,
                const uint64_t size,
                const uint64_t timestamp,
                const uint64_t packageSize);
        public:
            virtual ~ResourceInRepository();

            const QUrl url;
            const uint64_t size;
            const uint64_t timestamp;
            const uint64_t packageSize;

        friend class OsmAnd::ResourcesManager_P;
        };

        struct OSMAND_CORE_API ObfMetadata : public Resource::Metadata
        {
            ObfMetadata(const std::shared_ptr<const ObfFile>& obfFile);
            virtual ~ObfMetadata();

            const std::shared_ptr<const ObfFile> obfFile;

        private:
            Q_DISABLE_COPY_AND_MOVE(ObfMetadata);
        };

        struct OSMAND_CORE_API MapStyleMetadata : public Resource::Metadata
        {
            MapStyleMetadata(const std::shared_ptr<UnresolvedMapStyle>& mapStyle);
            virtual ~MapStyleMetadata();

            const std::shared_ptr<UnresolvedMapStyle> mapStyle;

        private:
            Q_DISABLE_COPY_AND_MOVE(MapStyleMetadata);
        };

        struct OSMAND_CORE_API OnlineTileSourcesMetadata : public Resource::Metadata
        {
            OnlineTileSourcesMetadata(const std::shared_ptr<const OnlineTileSources>& sources);
            virtual ~OnlineTileSourcesMetadata();

            const std::shared_ptr<const OnlineTileSources> sources;

        private:
            Q_DISABLE_COPY_AND_MOVE(OnlineTileSourcesMetadata);
        };
    private:
        PrivateImplementation<ResourcesManager_P> _p;
    protected:
    public:
        ResourcesManager(
            const QString& localStoragePath,
            const QString& userStoragePath = {},
            const QList<QString>& readonlyExternalStoragePaths = QList<QString>(),
            const QString& miniBasemapFilename = {},
            const QString& localTemporaryPath = {},
            const QString& localCachePath = {},
            const QString& appVersion = {},
            const QString& repositoryBaseUrl = QLatin1String("http://download.osmand.net"),
            const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient()));
        virtual ~ResourcesManager();

        const QString localStoragePath;
        const QString userStoragePath;
        const QList<QString> readonlyExternalStoragePaths;
        const QString miniBasemapFilename;
        const QString localTemporaryPath;
        const QString repositoryBaseUrl;
        const QString localCachePath;
        const QString appVersion;

        // Generic accessors:
        std::shared_ptr<const Resource> getResource(const QString& id) const;
        
        // Built-in resources:
        QHash< QString, std::shared_ptr<const BuiltinResource> > getBuiltInResources() const;
        std::shared_ptr<const BuiltinResource> getBuiltInResource(const QString& id) const;
        bool isBuiltInResource(const QString& id) const;

        // Local resources:
        bool rescanUnmanagedStoragePaths() const;
        QList< std::shared_ptr<const LocalResource> > getSortedLocalResources() const;
        QHash< QString, std::shared_ptr<const LocalResource> > getLocalResources() const;
        std::shared_ptr<const LocalResource> getLocalResource(const QString& id) const;
        bool isLocalResource(const QString& id) const;

        // Resources in repository:
        bool isRepositoryAvailable() const;
        bool updateRepository() const;
        QHash< QString, std::shared_ptr<const ResourceInRepository> > getResourcesInRepository() const;
        std::shared_ptr<const ResourceInRepository> getResourceInRepository(const QString& id) const;
        bool isResourceInRepository(const QString& id) const;

        // Install / Uninstall:
        bool isResourceInstalled(const QString& id) const;
        uint64_t getResourceTimestamp(const QString& id) const;
        bool uninstallResource(const QString& id);
        bool uninstallResource(const std::shared_ptr<const OsmAnd::ResourcesManager::InstalledResource> &installedResource, const std::shared_ptr<const OsmAnd::ResourcesManager::LocalResource> &resource);
        bool uninstallTilesResource(const QString& name);
        bool installTilesResource(const std::shared_ptr<const IOnlineTileSources::Source>& source);
        void installOsmAndOnlineTileSource();
        bool installFromFile(const QString& filePath, const ResourceType resourceType);
        bool installFromFile(const QString& id, const QString& filePath, const ResourceType resourceType);
        bool installImportedResource(const QString& filePath, const QString& newName, const ResourceType resourceType);
        bool installFromRepository(
            const QString& id,
            const IWebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);
        bool installFromRepository(const QString& id, const QString& filePath);

        // Updates:
        bool isInstalledResourceOutdated(const QString& id) const;
        QHash< QString, std::shared_ptr<const LocalResource> > getOutdatedInstalledResources() const;
        bool updateFromFile(const QString& filePath);
        bool updateFromFile(const QString& id, const QString& filePath);
        bool updateFromRepository(
            const QString& id,
            const IWebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);
        bool updateFromRepository(const QString& id, const QString& filePath);
        
        const std::shared_ptr<const OnlineTileSources> downloadOnlineTileSources() const;
        
        // Tests
        bool addLocalResource(const QString& filePath);

        // Observables
        OSMAND_OBSERVER_CALLABLE(LocalResourcesChanged,
            const ResourcesManager* const resourcesManager,
            const QList< QString >& added,
            const QList< QString >& removed,
            const QList< QString >& updated);
        const ObservableAs<ResourcesManager::LocalResourcesChanged> localResourcesChangeObservable;
        OSMAND_OBSERVER_CALLABLE(RepositoryUpdated,
            const ResourcesManager* const resourcesManager);
        const ObservableAs<ResourcesManager::RepositoryUpdated> repositoryUpdateObservable;

        const std::shared_ptr<const IOnlineTileSources>& onlineTileSources;
        const std::shared_ptr<const IMapStylesCollection>& mapStylesCollection;
        const std::shared_ptr<const IObfsCollection>& obfsCollection;
        const std::shared_ptr<IncrementalChangesManager>& changesManager;
        
        static OsmAnd::ResourcesManager::ResourceType getIndexType(const QStringRef &resourceTypeValue);
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_H_)
