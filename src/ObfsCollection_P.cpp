#include "ObfsCollection_P.h"
#include "ObfsCollection.h"

#include <cassert>

#include "QtCommon.h"

#include "OsmAndCore_private.h"
#include "ObfReader.h"
#include "ObfDataInterface.h"
#include "ObfFile.h"
#include "ObfInfo.h"
#include "QKeyValueIterator.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"
#include "CachedOsmandIndexes.h"

OsmAnd::ObfsCollection_P::ObfsCollection_P(ObfsCollection* owner_)
    : _fileSystemWatcher(new QFileSystemWatcher())
    , _lastUnusedSourceOriginId(0)
    , _collectedSourcesInvalidated(1)
    , owner(owner_)
{
    _fileSystemWatcher->moveToThread(gMainThread);

    _onDirectoryChangedConnection = QObject::connect(
        _fileSystemWatcher, &QFileSystemWatcher::directoryChanged,
        (std::function<void(const QString&)>)std::bind(&ObfsCollection_P::onDirectoryChanged, this, std::placeholders::_1));
    _onFileChangedConnection = QObject::connect(
        _fileSystemWatcher, &QFileSystemWatcher::fileChanged,
        (std::function<void(const QString&)>)std::bind(&ObfsCollection_P::onFileChanged, this, std::placeholders::_1));
}

OsmAnd::ObfsCollection_P::~ObfsCollection_P()
{
    QObject::disconnect(_onDirectoryChangedConnection);
    QObject::disconnect(_onFileChangedConnection);

    _fileSystemWatcher->deleteLater();
}

void OsmAnd::ObfsCollection_P::invalidateCollectedSources()
{
    _collectedSourcesInvalidated.fetchAndAddOrdered(1);
}

void OsmAnd::ObfsCollection_P::collectSources() const
{
    QWriteLocker scopedLocker1(&_collectedSourcesLock);
    QReadLocker scopedLocker2(&_sourcesOriginsLock);

    // Capture how many invalidations are going to be processed
    const auto invalidationsToProcess = _collectedSourcesInvalidated.loadAcquire();
    if (invalidationsToProcess == 0)
        return;

    const Stopwatch collectSourcesStopwatch(true);

    std::shared_ptr<CachedOsmandIndexes> cachedOsmandIndexes = nullptr;
    QFile* indCache = NULL;
    if (_sourcesOrigins.size() > 0)
    {
        auto origin = _sourcesOrigins.values()[0];
        if (origin->type == SourceOriginType::Directory)
        {
            const auto& directory = std::static_pointer_cast<const DirectoryAsSourceOrigin>(origin);
            indCache = new QFile(directory->directory.absoluteFilePath(QLatin1String("ind_core.cache")));
        }
    }
    if (indCache)
    {
        cachedOsmandIndexes = std::make_shared<CachedOsmandIndexes>();
        if (indCache->exists())
        {
            cachedOsmandIndexes->readFromFile(indCache->fileName(), CachedOsmandIndexes::VERSION);
        }
        else
        {
            indCache->open(QIODevice::ReadWrite);
            if (indCache->isOpen())
                indCache->close();
        }
    }
    
    // Check all previously collected sources
    auto itCollectedSourcesEntry = mutableIteratorOf(_collectedSources);
    while(itCollectedSourcesEntry.hasNext())
    {
        const auto& collectedSourcesEntry = itCollectedSourcesEntry.next();
        const auto& originId = collectedSourcesEntry.key();
        auto& collectedSources = collectedSourcesEntry.value();
        const auto& itSourceOrigin = _sourcesOrigins.constFind(originId);

        // If current source origin was removed,
        // remove entire each collected source related to it
        if (itSourceOrigin == _sourcesOrigins.cend())
        {
            // Ensure that ObfFile is not being read anywhere
            for(const auto& itCollectedSource : rangeOf(collectedSources))
            {
                const auto obfFile = itCollectedSource.value();

                //NOTE: OBF should have been locked here, but since file is gone anyways, this lock is quite useless

                itCollectedSource.value().reset();
                assert(obfFile.use_count() == 1);
            }

            itCollectedSourcesEntry.remove();
            continue;
        }

        // Check for missing files
        auto itObfFileEntry = mutableIteratorOf(collectedSources);
        while(itObfFileEntry.hasNext())
        {
            const auto& sourceFilename = itObfFileEntry.next().key();
            if (QFile::exists(sourceFilename))
                continue;
            const auto obfFile = itObfFileEntry.value();

            //NOTE: OBF should have been locked here, but since file is gone anyways, this lock is quite useless

            itObfFileEntry.remove();
            assert(obfFile.use_count() == 1);
        }

        // If all collected sources for current source origin are gone,
        // remove entire collection attached to source origin ID
        if (collectedSources.isEmpty())
        {
            itCollectedSourcesEntry.remove();
            continue;
        }
    }

    // Find all files uncollected sources
    for(const auto& itEntry : rangeOf(constOf(_sourcesOrigins)))
    {
        const auto& originId = itEntry.key();
        const auto& entry = itEntry.value();
        auto itCollectedSources = _collectedSources.find(originId);
        if (itCollectedSources == _collectedSources.end())
            itCollectedSources = _collectedSources.insert(originId, QHash<QString, std::shared_ptr<ObfFile> >());
        auto& collectedSources = *itCollectedSources;

        if (entry->type == SourceOriginType::Directory)
        {
            const auto& directoryAsSourceOrigin = std::static_pointer_cast<const DirectoryAsSourceOrigin>(entry);

            QFileInfoList obfFilesInfo;
            Utilities::findFiles(directoryAsSourceOrigin->directory, QStringList() << QLatin1String("*.obf"), obfFilesInfo, directoryAsSourceOrigin->isRecursive);
            for(const auto& obfFileInfo : constOf(obfFilesInfo))
            {
                const auto& obfFilePath = obfFileInfo.canonicalFilePath();
                if (collectedSources.constFind(obfFilePath) != collectedSources.cend())
                    continue;
                
                auto obfFile = cachedOsmandIndexes->getObfFile(obfFilePath);
                collectedSources.insert(obfFilePath, obfFile);
            }

            if (directoryAsSourceOrigin->isRecursive)
            {
                QFileInfoList directoriesInfo;
                Utilities::findDirectories(directoryAsSourceOrigin->directory, QStringList() << QLatin1String("*"), directoriesInfo, true);

                for(const auto& directoryInfo : constOf(directoriesInfo))
                {
                    const auto canonicalPath = directoryInfo.canonicalFilePath();
                    if (directoryAsSourceOrigin->watchedSubdirectories.contains(canonicalPath))
                        continue;

                    _fileSystemWatcher->addPath(canonicalPath);
                    directoryAsSourceOrigin->watchedSubdirectories.insert(canonicalPath);
                }
            }
        }
        else if (entry->type == SourceOriginType::File)
        {
            const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(entry);

            if (!fileAsSourceOrigin->fileInfo.exists())
                continue;
            const auto& obfFilePath = fileAsSourceOrigin->fileInfo.canonicalFilePath();
            if (collectedSources.constFind(obfFilePath) != collectedSources.cend())
                continue;

            auto obfFile = cachedOsmandIndexes->getObfFile(obfFilePath);
            collectedSources.insert(obfFilePath, obfFile);
        }
    }

    if (cachedOsmandIndexes && _collectedSources.size() > 0)
        cachedOsmandIndexes->writeToFile(indCache->fileName());
    if (indCache)
        delete indCache;

    // Decrement invalidations counter with number of processed onces
    _collectedSourcesInvalidated.fetchAndAddOrdered(-invalidationsToProcess);

    LogPrintf(LogSeverityLevel::Info, "Collected OBF sources in %fs", collectSourcesStopwatch.elapsed());
}

QList<OsmAnd::ObfsCollection::SourceOriginId> OsmAnd::ObfsCollection_P::getSourceOriginIds() const
{
    QReadLocker scopedLocker(&_sourcesOriginsLock);

    return _sourcesOrigins.keys();
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection_P::addDirectory(const QDir& dir, bool recursive)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto allocatedId = _lastUnusedSourceOriginId++;
    auto sourceOrigin = new DirectoryAsSourceOrigin();
    sourceOrigin->directory = dir;
    sourceOrigin->isRecursive = recursive;
    _sourcesOrigins.insert(allocatedId, qMove(std::shared_ptr<const SourceOrigin>(sourceOrigin)));

    _fileSystemWatcher->addPath(dir.canonicalPath());
    if (recursive)
    {
        QFileInfoList subdirs;
        Utilities::findDirectories(dir, QStringList() << QLatin1String("*"), subdirs, true);
        for(const auto& subdir : subdirs)
        {
            const auto canonicalPath = subdir.canonicalFilePath();
            sourceOrigin->watchedSubdirectories.insert(canonicalPath);
            _fileSystemWatcher->addPath(canonicalPath);
        }
    }

    invalidateCollectedSources();

    return allocatedId;
}

OsmAnd::ObfsCollection::SourceOriginId OsmAnd::ObfsCollection_P::addFile(const QFileInfo& fileInfo)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto allocatedId = _lastUnusedSourceOriginId++;
    auto sourceOrigin = new ObfsCollection_P::FileAsSourceOrigin();
    sourceOrigin->fileInfo = fileInfo;
    _sourcesOrigins.insert(allocatedId, qMove(std::shared_ptr<const SourceOrigin>(sourceOrigin)));

    _fileSystemWatcher->addPath(fileInfo.canonicalFilePath());

    invalidateCollectedSources();

    return allocatedId;
}

bool OsmAnd::ObfsCollection_P::remove(const ObfsCollection::SourceOriginId entryId)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto itSourceOrigin = _sourcesOrigins.find(entryId);
    if (itSourceOrigin == _sourcesOrigins.end())
        return false;

    const auto& sourceOrigin = *itSourceOrigin;
    if (sourceOrigin->type == SourceOriginType::Directory)
    {
        const auto& directoryAsSourceOrigin = std::static_pointer_cast<const DirectoryAsSourceOrigin>(sourceOrigin);

        for(const auto& watchedSubdirectory : constOf(directoryAsSourceOrigin->watchedSubdirectories))
            _fileSystemWatcher->removePath(watchedSubdirectory);
        _fileSystemWatcher->removePath(directoryAsSourceOrigin->directory.canonicalPath());
    }
    else if (sourceOrigin->type == SourceOriginType::File)
    {
        const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(sourceOrigin);

        _fileSystemWatcher->removePath(fileAsSourceOrigin->fileInfo.canonicalFilePath());
    }

    _sourcesOrigins.erase(itSourceOrigin);

    invalidateCollectedSources();

    return true;
}

QList< std::shared_ptr<const OsmAnd::ObfFile> > OsmAnd::ObfsCollection_P::getObfFiles() const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();

    QList< std::shared_ptr<const OsmAnd::ObfFile> > obfFiles;
    {
        QReadLocker scopedLocker(&_collectedSourcesLock);

        for(const auto& collectedSources : constOf(_collectedSources))
        {
            obfFiles.reserve(obfFiles.size() + collectedSources.size());
            for(const auto& obfFile : collectedSources)
                obfFiles.append(obfFile);
        }
    }
    return obfFiles;
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection_P::obtainDataInterface(
    const std::shared_ptr<const ObfFile> obfFile) const
{
    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface({ std::make_shared<ObfReader>(obfFile) }));
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection_P::obtainDataInterface(
    const QList< std::shared_ptr<const ResourcesManager::LocalResource> > localResources) const
{
    QList< std::shared_ptr<const ObfReader> > obfReaders;
    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface(obfReaders));
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection_P::obtainDataInterface(
    const AreaI* const pBbox31 /*= nullptr*/,
    const ZoomLevel minZoomLevel /*= MinZoomLevel*/,
    const ZoomLevel maxZoomLevel /*= MaxZoomLevel*/,
    const ObfDataTypesMask desiredDataTypes /*= fullObfDataTypesMask()*/) const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();

    // Create ObfReaders from collected sources
    QList< std::shared_ptr<const ObfReader> > obfReaders;
    {
        QReadLocker scopedLocker(&_collectedSourcesLock);

        for (const auto& collectedSources : constOf(_collectedSources))
        {
            obfReaders.reserve(obfReaders.size() + collectedSources.size());
            for (const auto& obfFile : constOf(collectedSources))
            {
                // If OBF information already available, perform check
                if (obfFile->obfInfo &&
                    !obfFile->obfInfo->isBasemap &&
                    !obfFile->obfInfo->isBasemapWithCoastlines)
                {
                    bool accept = obfFile->obfInfo->containsDataFor(pBbox31, minZoomLevel, maxZoomLevel, desiredDataTypes);
                    if (!accept)
                        continue;
                }

                // Otherwise, open file in any case to repeat check
                std::shared_ptr<const ObfReader> obfReader(new ObfReader(obfFile));
                if (!obfReader->isOpened() || !obfReader->obtainInfo())
                    continue;

                // Repeat checks if needed
                if (!obfFile->obfInfo->isBasemap && !obfFile->obfInfo->isBasemapWithCoastlines)
                {
                    bool accept = obfFile->obfInfo->containsDataFor(pBbox31, minZoomLevel, maxZoomLevel, desiredDataTypes);
                    if (!accept)
                        continue;
                }

                obfReaders.push_back(qMove(obfReader));
            }
        }
    }

    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface(obfReaders));
}

void OsmAnd::ObfsCollection_P::onDirectoryChanged(const QString& path)
{
    invalidateCollectedSources();
}

void OsmAnd::ObfsCollection_P::onFileChanged(const QString& path)
{
    invalidateCollectedSources();
}
