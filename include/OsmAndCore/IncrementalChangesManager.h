#ifndef _OSMAND_CORE_INCREMENTAL_CHANGES_MANAGER_H_
#define _OSMAND_CORE_INCREMENTAL_CHANGES_MANAGER_H_

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
#include <OsmAndCore/ResourcesManager.h>

namespace OsmAnd
{
    class IncrementalChangesManager_P;
    class OSMAND_CORE_API IncrementalChangesManager
    {
        Q_DISABLE_COPY_AND_MOVE(IncrementalChangesManager);
    public:
        class OSMAND_CORE_API RegionUpdateFiles
        {
            Q_DISABLE_COPY_AND_MOVE(RegionUpdateFiles);
        protected:
            RegionUpdateFiles(QString& name, std::shared_ptr<const ResourcesManager::InstalledResource> mainFile);
            
        private:
            
        public:
            virtual ~RegionUpdateFiles();
            QString name;
            std::shared_ptr<const ResourcesManager::InstalledResource> mainFile;
            QHash<QString, QList< std::shared_ptr<const ResourcesManager::InstalledResource> > > dailyUpdates;
            QHash<QString, std::shared_ptr<const ResourcesManager::InstalledResource> > monthlyUpdates;
            bool addUpdate(std::shared_ptr<const ResourcesManager::InstalledResource>);
            uint64_t getTimestamp() const;
            bool isEmpty() const;
            
            friend class IncrementalChangesManager_P;
        };
        
        class OSMAND_CORE_API IncrementalUpdate
        {
        protected:
            
            QString sizeText;
        private:
            
        public:
            virtual ~IncrementalUpdate();
            
            QString date;
            uint64_t timestamp;
            long containerSize;
            long contentSize;
            QString fileName;
            QUrl url;
            QString resId;
            
            bool isMonth() const;
            
            friend class IncrementalChangesManager_P;
        };
        
        class OSMAND_CORE_API IncrementalUpdateGroupByMonth
        {

        protected:

        private:

        public:
            virtual ~IncrementalUpdateGroupByMonth();
            
            bool monthUpdateInitialized = false;
            QList<std::shared_ptr<const IncrementalUpdate> > dayUpdates;
            std::shared_ptr<const IncrementalUpdate> monthUpdate;
            QString monthYearPart;
            bool isMonthUpdateApplicable() const;
            bool isDayUpdateApplicable() const;
            QList<std::shared_ptr<const IncrementalUpdate> > getMonthUpdate() const;
            
            
            friend class IncrementalChangesManager_P;
        };
        
        class OSMAND_CORE_API IncrementalUpdateList
        {

        protected:
            
        private:
            
        public:
            QHash<QString, std::shared_ptr<IncrementalUpdateGroupByMonth> > updatesByMonth;
            QString errorMessage;
            std::shared_ptr<const RegionUpdateFiles> updateFiles;
            
            bool isPreferrableLimitForDayUpdates(const QString &monthYearPart) const;
            void addUpdate(const std::shared_ptr<const IncrementalUpdate>& update);
            QList<std::shared_ptr<const IncrementalUpdate> > getItemsForUpdate() const;
            
        public:
            virtual ~IncrementalUpdateList();
            
            friend class IncrementalChangesManager_P;
        };

    private:
        PrivateImplementation<IncrementalChangesManager_P> _p;
        IncrementalChangesManager(
                                  const std::shared_ptr<const IWebClient>& webClient = std::shared_ptr<const IWebClient>(new WebClient()),
                                  ResourcesManager* resourcesManager = nullptr);
        void onLocalResourcesChanged(const QList< QString >& added, const QList< QString >& removed);
        
    protected:
    public:
        
        virtual ~IncrementalChangesManager();

        const QString repositoryBaseUrl;
        
        bool addValidIncrementalUpdates(QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &liveResources,
                                        QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &mapResources);
        
        std::shared_ptr<const IncrementalUpdateList> getUpdatesByMonth(const QString& regionName) const;
        void deleteUpdates(const QString &regionName);
        uint64_t getUpdatesSize(const QString& regionName);
    friend class OsmAnd::ResourcesManager_P;
    };
}

#endif // !defined(_OSMAND_CORE_RESOURCES_MANAGER_H_)
