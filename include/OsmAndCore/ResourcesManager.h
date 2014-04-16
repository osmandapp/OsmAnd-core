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
#include <OsmAndCore/WebClient.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Data/ObfInfo.h>

namespace OsmAnd
{
    class IObfsCollection;

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
        };

        class OSMAND_CORE_API Resource
        {
        private:
        protected:
            Resource(
                const QString& name,
                const ResourceType type,
                const uint64_t timestamp,
                const uint64_t contentSize);
        public:
            virtual ~Resource();

            const QString name;
            const ResourceType type;
            const uint64_t timestamp;
            const uint64_t contentSize;

        friend class OsmAnd::ResourcesManager;
        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API LocalResource : public Resource
        {
        private:
        protected:
            LocalResource(
                const QString& name,
                const ResourceType type,
                const uint64_t timestamp,
                const uint64_t contentSize,
                const QString& localPath);
        public:
            virtual ~LocalResource();

            const QString localPath;

        friend class OsmAnd::ResourcesManager;
        friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API LocalObfResource : public LocalResource
        {
        private:
        protected:
            LocalObfResource(
                const QString& name,
                const ResourceType type,
                const std::shared_ptr<const ObfFile>& obfFile);
        public:
            virtual ~LocalObfResource();

            const std::shared_ptr<const ObfFile> obfFile;

            friend class OsmAnd::ResourcesManager;
            friend class OsmAnd::ResourcesManager_P;
        };

        class OSMAND_CORE_API ResourceInRepository : public Resource
        {
        private:
        protected:
            ResourceInRepository(
                const QString& name,
                const ResourceType type,
                const uint64_t timestamp,
                const uint64_t contentSize,
                const QString& containerDownloadUrl,
                const uint64_t containerSize);
        public:
            virtual ~ResourceInRepository();

            const QString containerDownloadUrl;
            const uint64_t containerSize;

        friend class OsmAnd::ResourcesManager;
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
        bool installFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);

        // Updates:
        bool updateAvailableInRepositoryFor(const QString& name) const;
        QList<QString> getAvailableUpdatesFromRepository() const;
        bool updateFromFile(const QString& filePath);
        bool updateFromFile(const QString& name, const QString& filePath);
        bool updateFromRepository(const QString& name, const WebClient::RequestProgressCallbackSignature downloadProgressCallback = nullptr);

        // OBFs collection
        const std::shared_ptr<const IObfsCollection>& obfsCollection;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_H_)
