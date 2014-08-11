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
#include <OsmAndCore/WebClient.h>
#include <OsmAndCore/AccessLockCounter.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>

namespace OsmAnd
{
    class IOnlineTileSources;
    class IMapStylesCollection;
    class IMapStylesPresetsCollection;
    class IObfsCollection;
    class MapStyle;
    class MapStylesPresetsCollection;
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
            VoicePack,
            MapStyle,
            MapStylesPresets,
            OnlineTileSources,
            //RoadMapRegion,
            //SrtmRegion,
            //HillshadeRegion,
            //HeightmapRegion
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
        };

        struct OSMAND_CORE_API MapStyleMetadata : public Resource::Metadata
        {
            MapStyleMetadata(const std::shared_ptr<MapStyle>& mapStyle);
            virtual ~MapStyleMetadata();

            const std::shared_ptr<MapStyle> mapStyle;
        };

        struct OSMAND_CORE_API MapStylesPresetsMetadata : public Resource::Metadata
        {
            MapStylesPresetsMetadata(const std::shared_ptr<const MapStylesPresetsCollection>& presets);
            virtual ~MapStylesPresetsMetadata();

            const std::shared_ptr<const MapStylesPresetsCollection> presets;
        };

        struct OSMAND_CORE_API OnlineTileSourcesMetadata : public Resource::Metadata
        {
            OnlineTileSourcesMetadata(const std::shared_ptr<const OnlineTileSources>& sources);
            virtual ~OnlineTileSourcesMetadata();

            const std::shared_ptr<const OnlineTileSources> sources;
        };
    private:
        PrivateImplementation<ResourcesManager_P> _p;
    protected:
    public:
        ResourcesManager(
            const QString& localStoragePath,
            const QString& userStoragePath = QString::null,
            const QList<QString>& readonlyExternalStoragePaths = QList<QString>(),
            const QString& miniBasemapFilename = QString::null,
            const QString& localTemporaryPath = QString::null,
            const QString& repositoryBaseUrl = QLatin1String("http://download.osmand.net"));
        virtual ~ResourcesManager();

        const QString localStoragePath;
        const QString userStoragePath;
        const QList<QString> readonlyExternalStoragePaths;
        const QString miniBasemapFilename;
        const QString localTemporaryPath;
        const QString repositoryBaseUrl;

        // Generic accessors:
        std::shared_ptr<const Resource> getResource(const QString& id) const;
        
        // Built-in resources:
        QHash< QString, std::shared_ptr<const BuiltinResource> > getBuiltInResources() const;
        std::shared_ptr<const BuiltinResource> getBuiltInResource(const QString& id) const;
        bool isBuiltInResource(const QString& id) const;

        // Local resources:
        bool rescanUnmanagedStoragePaths() const;
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
        bool uninstallResource(const QString& id);
        bool installFromFile(const QString& filePath, const ResourceType resourceType);
        bool installFromFile(const QString& id, const QString& filePath, const ResourceType resourceType);
        bool installFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);
        bool installFromRepository(const QString& id, const QString& filePath);

        // Updates:
        bool isInstalledResourceOutdated(const QString& id) const;
        QHash< QString, std::shared_ptr<const LocalResource> > getOutdatedInstalledResources() const;
        bool updateFromFile(const QString& filePath);
        bool updateFromFile(const QString& id, const QString& filePath);
        bool updateFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);
        bool updateFromRepository(const QString& id, const QString& filePath);

        // Observables
        OSMAND_CALLABLE(LocalResourcesChanged,
            void,
            const ResourcesManager* const resourcesManager,
            const QList< QString >& added,
            const QList< QString >& removed,
            const QList< QString >& updated);
        const ObservableAs<LocalResourcesChanged> localResourcesChangeObservable;
        OSMAND_CALLABLE(RepositoryUpdated,
            void,
            const ResourcesManager* const resourcesManager);
        const ObservableAs<RepositoryUpdated> repositoryUpdateObservable;

        const std::shared_ptr<const IOnlineTileSources>& onlineTileSources;
        const std::shared_ptr<const IMapStylesCollection>& mapStylesCollection;
        const std::shared_ptr<const IMapStylesPresetsCollection>& mapStylesPresetsCollection;
        const std::shared_ptr<const IObfsCollection>& obfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_H_)
