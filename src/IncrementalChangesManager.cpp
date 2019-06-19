#include "IncrementalChangesManager.h"
#include "IncrementalChangesManager_P.h"

#include <QStandardPaths>
#include <QDir>

#include "ObfInfo.h"

#define DAILY_UPDATES_LIMIT 10

OsmAnd::IncrementalChangesManager::IncrementalChangesManager(
    const std::shared_ptr<const IWebClient>& webClient /*= std::shared_ptr<const IWebClient>(new WebClient())*/,
    ResourcesManager* resourcesManager)
    : _p(new IncrementalChangesManager_P(this, webClient, resourcesManager))
    , repositoryBaseUrl(QStringLiteral("https://download.osmand.net/"))
{

    _p->initialize();
}

OsmAnd::IncrementalChangesManager::~IncrementalChangesManager()
{
}

bool OsmAnd::IncrementalChangesManager::addValidIncrementalUpdates(QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &liveResources,
                                                                   QHash< QString, std::shared_ptr<const ResourcesManager::LocalResource> > &mapResources)
{
    return _p->addValidIncrementalUpdates(liveResources, mapResources);
}

std::shared_ptr<const OsmAnd::IncrementalChangesManager::IncrementalUpdateList> OsmAnd::IncrementalChangesManager::getUpdatesByMonth(const QString& regionName) const
{
    return _p->getUpdatesByMonth(regionName);
}

void OsmAnd::IncrementalChangesManager::deleteUpdates(const QString &regionName)
{
    _p->deleteUpdates(regionName);
}

void OsmAnd::IncrementalChangesManager::onLocalResourcesChanged(const QList< QString >& added, const QList< QString >& removed)
{
    _p->onLocalResourcesChanged(added, removed);
}

uint64_t OsmAnd::IncrementalChangesManager::getUpdatesSize(const QString& regionName)
{
    return _p->getUpdatesSize(regionName);
}

uint64_t OsmAnd::IncrementalChangesManager::getUpdatesSize()
{
    return _p->getUpdatesSize();
}

OsmAnd::IncrementalChangesManager::RegionUpdateFiles::RegionUpdateFiles(QString& name, std::shared_ptr<const ResourcesManager::InstalledResource> mainFile):
    name(name),
    mainFile(mainFile)
{
}

OsmAnd::IncrementalChangesManager::RegionUpdateFiles::~RegionUpdateFiles()
{
}

bool OsmAnd::IncrementalChangesManager::RegionUpdateFiles::addUpdate(std::shared_ptr<const OsmAnd::ResourcesManager::InstalledResource> liveResource)
{
    QString resId = liveResource->id;
    QString fileWithoutExtenion = resId.remove(QStringLiteral(".live.obf"));
    QString date = fileWithoutExtenion.mid(fileWithoutExtenion.length() - 8);
    if (fileWithoutExtenion.endsWith(QStringLiteral("_00")))
        monthlyUpdates.insert(date, liveResource);
    else
    {
        QList< std::shared_ptr<const ResourcesManager::InstalledResource> > updateList = dailyUpdates.value(date);
        updateList.append(liveResource);
        dailyUpdates.insert(date, updateList);
    }
    
    return true;
}

uint64_t OsmAnd::IncrementalChangesManager::RegionUpdateFiles::getTimestamp() const
{
    uint64_t timestamp = mainFile->timestamp;
    for (const auto& monthlyUpdate : monthlyUpdates.values())
    {
        timestamp = qMax(timestamp, monthlyUpdate->timestamp);
    }
    for (const auto& dailyUpdateList : dailyUpdates.values())
    {
        for (const auto& dailyUpdate : dailyUpdateList)
        {
            timestamp = qMax(timestamp, dailyUpdate->timestamp);
        }
    }
    return timestamp;
}

bool OsmAnd::IncrementalChangesManager::RegionUpdateFiles::isEmpty() const
{
    return dailyUpdates.isEmpty() && monthlyUpdates.isEmpty();
}

OsmAnd::IncrementalChangesManager::IncrementalUpdate::~IncrementalUpdate()
{
}

bool OsmAnd::IncrementalChangesManager::IncrementalUpdate::isMonth() const
{
    return date.endsWith(QStringLiteral("_00"));
}

OsmAnd::IncrementalChangesManager::IncrementalUpdateGroupByMonth::~IncrementalUpdateGroupByMonth()
{
}

bool OsmAnd::IncrementalChangesManager::IncrementalUpdateGroupByMonth::isMonthUpdateApplicable() const
{
    return monthUpdateInitialized;
}

bool OsmAnd::IncrementalChangesManager::IncrementalUpdateGroupByMonth::isDayUpdateApplicable() const
{
    return dayUpdates.size() > 0 && dayUpdates.size() < 4;
}

QList<std::shared_ptr<const OsmAnd::IncrementalChangesManager::IncrementalUpdate> > OsmAnd::IncrementalChangesManager::IncrementalUpdateGroupByMonth::getMonthUpdate() const
{
    QList<std::shared_ptr<const IncrementalUpdate> > result;
    if(!monthUpdateInitialized) {
        return result;
    }
    result.append(monthUpdate);
    for(const auto& dayUpdate : dayUpdates) {
        if(dayUpdate->timestamp > monthUpdate->timestamp)
            result.append(dayUpdate);
    }
    return result;
}

OsmAnd::IncrementalChangesManager_P::IncrementalUpdateList::~IncrementalUpdateList()
{
}

bool OsmAnd::IncrementalChangesManager::IncrementalUpdateList::isPreferrableLimitForDayUpdates(const QString &monthYearPart) const
{
    
    QHash<QString, QList<std::shared_ptr<const ResourcesManager::InstalledResource> > > dailyUpdates = updateFiles->dailyUpdates;
    QList<std::shared_ptr<const ResourcesManager::InstalledResource> > lst = dailyUpdates.value(monthYearPart);
    
    return lst.size() < DAILY_UPDATES_LIMIT;
}

QList<std::shared_ptr<const OsmAnd::IncrementalChangesManager::IncrementalUpdate> > OsmAnd::IncrementalChangesManager::IncrementalUpdateList::getItemsForUpdate() const
{
    QListIterator<std::shared_ptr<IncrementalUpdateGroupByMonth> > it(updatesByMonth.values());
    QList<std::shared_ptr<const IncrementalUpdate> > result;
    
    while(it.hasNext()) {
        const auto& n = it.next();
        if(it.hasNext()) {
            if(!n->isMonthUpdateApplicable()) {
                return result;
            }
            for (const auto& monthUpdate : n->getMonthUpdate())
            {
                result.append(monthUpdate);
            }
        } else {
            if(n->isDayUpdateApplicable() && isPreferrableLimitForDayUpdates(n->monthYearPart))
            {
                for (const auto& dayUpdate : n->dayUpdates)
                {
                    result.append(dayUpdate);
                }
            } else if(n->isMonthUpdateApplicable()) {
                for (const auto& monthUpdate : n->getMonthUpdate())
                {
                    result.append(monthUpdate);
                }
            } else {
                return result;
            }
        }
    }
    return result;
}

void OsmAnd::IncrementalChangesManager::IncrementalUpdateList::addUpdate(const std::shared_ptr<const IncrementalUpdate>& update)
{
    QString dtMonth = update->date;
    dtMonth = dtMonth.mid(0, 5);
    if(!updatesByMonth.contains(dtMonth)) {
        IncrementalUpdateGroupByMonth updateByMonth;
        updateByMonth.monthYearPart = dtMonth;
        updatesByMonth.insert(dtMonth, std::make_shared<IncrementalUpdateGroupByMonth>(updateByMonth));
    }
    std::shared_ptr<IncrementalUpdateGroupByMonth> mm = updatesByMonth.value(dtMonth);
    if(update->isMonth())
    {
        mm->monthUpdate = std::shared_ptr<const IncrementalUpdate>(update);
        mm->monthUpdateInitialized = true;
    }
    else
        mm->dayUpdates.append(std::shared_ptr<const IncrementalUpdate>(update));
}


