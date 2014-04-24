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
    class IMapStylesCollection;
    class IObfsCollection;
    class MapStyle;
    class MapStylesPresets;
    class OnlineTileSources;

    class ResourcesManager_P;
    class OSMAND_CORE_API ResourcesManager
    {
        Q_DISABLE_COPY(ResourcesManager);
    public:
        enum class ResourceType
        {
            Unknown = -1,

            MapRegion,
            VoicePack,
            MapStyle,
            MapStylePresets,
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
            Q_DISABLE_COPY(Resource);
        private:
        protected:
            Resource(
                const QString& id,
                const ResourceType type,
                const ResourceOrigin origin);

            struct OSMAND_CORE_API Metadata
            {
                Metadata();
                virtual ~Metadata();
            };
            std::shared_ptr<Metadata> _metadata;
        public:
            virtual ~Resource();

            const QString id;
            const ResourceType type;
            const ResourceOrigin origin;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API LocalResource : public Resource
        {
            Q_DISABLE_COPY(LocalResource);
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
            Q_DISABLE_COPY(UnmanagedResource);
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
            Q_DISABLE_COPY(InstalledResource);
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
            Q_DISABLE_COPY(BuiltinResource);
        private:
        protected:
            BuiltinResource(
                const QString& id,
                const ResourceType type);
        public:
            virtual ~BuiltinResource();

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API BuiltinMapStyleResource : public BuiltinResource
        {
            Q_DISABLE_COPY(BuiltinMapStyleResource);
        private:
        protected:
            BuiltinMapStyleResource(
                const QString& id,
                const std::shared_ptr<const MapStyle>& style);
        public:
            virtual ~BuiltinMapStyleResource();

            const std::shared_ptr<const MapStyle> style;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API BuiltinMapStylesPresetsResource : public BuiltinResource
        {
            Q_DISABLE_COPY(BuiltinMapStylesPresetsResource);
        private:
        protected:
            BuiltinMapStylesPresetsResource(
                const QString& id,
                const std::shared_ptr<const MapStylesPresets>& presets);
        public:
            virtual ~BuiltinMapStylesPresetsResource();

            const std::shared_ptr<const MapStylesPresets> presets;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API BuiltinOnlineTileSourcesResource : public BuiltinResource
        {
            Q_DISABLE_COPY(BuiltinOnlineTileSourcesResource);
        private:
        protected:
            BuiltinOnlineTileSourcesResource(
                const QString& id,
                const std::shared_ptr<const OnlineTileSources>& sources);
        public:
            virtual ~BuiltinOnlineTileSourcesResource();

            const std::shared_ptr<const OnlineTileSources> sources;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API ResourceInRepository : public Resource
        {
            Q_DISABLE_COPY(ResourceInRepository);
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
        QList< std::shared_ptr<const ResourceInRepository> > getResourcesInRepository() const;
        std::shared_ptr<const ResourceInRepository> getResourceInRepository(const QString& id) const;
        bool isResourceInRepository(const QString& id) const;

        // Install / Uninstall:
        bool isResourceInstalled(const QString& id) const;
        bool uninstallResource(const QString& id);
        bool installFromFile(const QString& filePath, const ResourceType resourceType);
        bool installFromFile(const QString& id, const QString& filePath, const ResourceType resourceType);
        bool installFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);

        // Updates:
        bool isInstalledResourceOutdated(const QString& id) const;
        QList<QString> getOutdatedInstalledResources() const;
        bool updateFromFile(const QString& filePath);
        bool updateFromFile(const QString& id, const QString& filePath);
        bool updateFromRepository(const QString& id, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);

        // Observables
        OSMAND_CALLABLE(LocalResourcesChanged,
            void,
            const ResourcesManager* const resourcesManager,
            const QList< QString >& added,
            const QList< QString >& removed,
            const QList< QString >& updated);
        const ObservableAs<LocalResourcesChanged> localResourcesChangeObservable;

        const std::shared_ptr<const IMapStylesCollection>& mapStylesCollection;
        const std::shared_ptr<const IObfsCollection>& obfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_H_)
