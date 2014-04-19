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
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>

namespace OsmAnd
{
    class IObfsCollection;
    class MapStylesPresets;
    class OnlineTileSources;

    class ResourcesManager_P;
    class OSMAND_CORE_API ResourcesManager
    {
        Q_DISABLE_COPY(ResourcesManager);
    public:
        enum class ResourceOriginType
        {
            Builtin,
            Installed,
            Repository,
            User
        };

        class OSMAND_CORE_API ResourceOrigin
        {
            Q_DISABLE_COPY(ResourceOrigin);
        private:
        protected:
        public:
            ResourceOrigin(const ResourceOriginType type);
            virtual ~ResourceOrigin();

            const ResourceOriginType type;
        };

        class OSMAND_CORE_API LocalResourceOrigin : public ResourceOrigin
        {
            Q_DISABLE_COPY(LocalResourceOrigin);
        private:
        protected:
        public:
            LocalResourceOrigin(
                const ResourceOriginType type,
                const QString& localPath,
                const uint64_t size);
            virtual ~LocalResourceOrigin();

            const QString localPath;
            const uint64_t size;
        };

        class OSMAND_CORE_API UserResourceOrigin : public LocalResourceOrigin
        {
            Q_DISABLE_COPY(UserResourceOrigin);
        private:
        protected:
        public:
            UserResourceOrigin(
                const QString& localPath,
                const uint64_t size,
                const QString& name);
            virtual ~UserResourceOrigin();

            const QString name;
        };

        class OSMAND_CORE_API InstalledResourceOrigin : public LocalResourceOrigin
        {
            Q_DISABLE_COPY(InstalledResourceOrigin);
        private:
        protected:
        public:
            InstalledResourceOrigin(
                const QString& localPath,
                const uint64_t size,
                const uint64_t timestamp);
            virtual ~InstalledResourceOrigin();

            const uint64_t timestamp;
        };

        class OSMAND_CORE_API RepositoryResourceOrigin : public ResourceOrigin
        {
            Q_DISABLE_COPY(RepositoryResourceOrigin);
        private:
        protected:
        public:
            RepositoryResourceOrigin(
                const QUrl& url,
                const uint64_t size,
                const uint64_t timestamp,
                const uint64_t packageSize);
            virtual ~RepositoryResourceOrigin();

            const QUrl url;
            const uint64_t size;
            const uint64_t timestamp;
            const uint64_t packageSize;
        };

        enum class ResourceType
        {
            Unknown = -1,

            MapRegion,
            VoicePack,
            MapStylePresets,
            OnlineTileSources,
            //RoadMapRegion,
            //SrtmRegion,
            //HillshadeRegion,
            //HeightmapRegion
        };

        class OSMAND_CORE_API Resource
        {
            Q_DISABLE_COPY(Resource);
        private:
        protected:
            Resource(
                const QString& id,
                const ResourceType type,
                const std::shared_ptr<const ResourceOrigin>& origin);
        public:
            virtual ~Resource();

            const QString id;
            const ResourceType type;
            const std::shared_ptr<const ResourceOrigin> origin;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API LocalObfResource : public Resource
        {
            Q_DISABLE_COPY(LocalObfResource);
        private:
        protected:
            LocalObfResource(
                const QString& id,
                const ResourceType type,
                const std::shared_ptr<const ResourceOrigin>& origin,
                const std::shared_ptr<const ObfFile>& obfFile);
        public:
            virtual ~LocalObfResource();

            const std::shared_ptr<const ObfFile> obfFile;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API MapStylesPresetsResource : public Resource
        {
            Q_DISABLE_COPY(MapStylesPresetsResource);
        private:
        protected:
            MapStylesPresetsResource(
                const QString& id,
                const std::shared_ptr<const ResourceOrigin>& origin);
            MapStylesPresetsResource(
                const QString& id,
                const std::shared_ptr<const MapStylesPresets>& presets);
        public:
            virtual ~MapStylesPresetsResource();

            const std::shared_ptr<const MapStylesPresets> presets;

        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API OnlineTileSourcesResource : public Resource
        {
            Q_DISABLE_COPY(OnlineTileSourcesResource);
        private:
        protected:
            OnlineTileSourcesResource(
                const QString& id,
                const std::shared_ptr<const ResourceOrigin>& origin);
            OnlineTileSourcesResource(
                const QString& id,
                const std::shared_ptr<const OnlineTileSources>& sources);
        public:
            virtual ~OnlineTileSourcesResource();

            const std::shared_ptr<const OnlineTileSources> sources;

        friend class OsmAnd::ResourcesManager_P;
        };
    private:
        PrivateImplementation<ResourcesManager_P> _p;
    protected:
    public:
        ResourcesManager(
            const QString& localStoragePath,
            const QList<QString>& extraStoragePaths = QList<QString>(),
            const QString& miniBasemapFilename = QString::null,
            const QString& localTemporaryPath = QString::null,
            const QString& repositoryBaseUrl = QLatin1String("http://download.osmand.net"));
        virtual ~ResourcesManager();

        const QString localStoragePath;
        const QList<QString> extraStoragePaths;
        const QString miniBasemapFilename;
        const QString localTemporaryPath;
        const QString repositoryBaseUrl;
        
        // Built-in resources:
        QList< std::shared_ptr<const Resource> > getBuiltInResources() const;
        std::shared_ptr<const Resource> getBuiltInResource(const QString& id) const;
        bool isBuiltInResource(const QString& id) const;

        // Local resources:
        bool rescanLocalStoragePaths() const;
        QList< std::shared_ptr<const Resource> > getLocalResources() const;
        std::shared_ptr<const Resource> getLocalResource(const QString& id) const;
        bool isLocalResource(const QString& id) const;

        // Resources in repository:
        bool isRepositoryAvailable() const;
        bool updateRepository() const;
        QList< std::shared_ptr<const Resource> > getResourcesInRepository() const;
        std::shared_ptr<const Resource> getResourceInRepository(const QString& id) const;
        bool isResourceInRepository(const QString& id) const;

        /*
        // Install / Uninstall:
        bool uninstallResource(const QString& name);
        bool installFromFile(const QString& filePath, const ResourceType resourceType);
        bool installFromFile(const QString& name, const QString& filePath, const ResourceType resourceType);
        bool installFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);

        // Updates:
        bool updateAvailableInRepositoryFor(const QString& name) const;
        QList<QString> getAvailableUpdatesFromRepository() const;
        bool updateFromFile(const QString& filePath);
        bool updateFromFile(const QString& name, const QString& filePath);
        bool updateFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);
        */
        // Observables
        OSMAND_CALLABLE(LocalResourcesChanged, void, const ResourcesManager* const resourcesManager);
        const Observable<const ResourcesManager* const /*resourcesManager*/> localResourcesChangeObservable;

        // OBFs collection
        const std::shared_ptr<const IObfsCollection>& obfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_H_)
