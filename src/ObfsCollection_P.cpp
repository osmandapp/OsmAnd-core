#include "ObfsCollection_P.h"
#include "ObfsCollection.h"

#include <cassert>
#if OSMAND_DEBUG
#   include <chrono>
#endif

#include "OsmAndCore_private.h"
#include "ObfReader.h"
#include "ObfDataInterface.h"
#include "ObfFile.h"
#include "QKeyValueIterator.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::ObfsCollection_P::ObfsCollection_P(ObfsCollection* owner_)
    : owner(owner_)
    , _fileSystemWatcher(new QFileSystemWatcher())
    , _lastUnusedSourceOriginId(0)
    , _collectedSourcesInvalidated(1)
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
    if(invalidationsToProcess == 0)
        return;

#if OSMAND_DEBUG
    const auto collectSources_Begin = std::chrono::high_resolution_clock::now();
#endif

    // Check all previously collected sources
    QMutableHashIterator< ObfsCollection::SourceOriginId, QHash<QString, std::shared_ptr<ObfFile> > > itCollectedSourcesEntry(_collectedSources);
    while(itCollectedSourcesEntry.hasNext())
    {
        const auto& collectedSourcesEntry = itCollectedSourcesEntry.next();
        const auto& originId = collectedSourcesEntry.key();
        auto& collectedSources = collectedSourcesEntry.value();
        const auto& itSourceOrigin = _sourcesOrigins.constFind(originId);

        // If current source origin was removed,
        // remove entire each collected source related to it
        if(itSourceOrigin == _sourcesOrigins.cend())
        {
            // Ensure that ObfFile is not being read anywhere
            for(const auto& itCollectedSource : rangeOf(collectedSources))
            {
                const auto obfFile = itCollectedSource.value();

                obfFile->lockForRemoval();

                itCollectedSource.value().reset();
                assert(obfFile.use_count() == 1);
            }

            itCollectedSourcesEntry.remove();
            continue;
        }

        // Check for missing files
        QMutableHashIterator< QString, std::shared_ptr<ObfFile> > itObfFileEntry(collectedSources);
        while(itObfFileEntry.hasNext())
        {
            const auto& sourceFilename = itObfFileEntry.next().key();
            if(QFile::exists(sourceFilename))
                continue;
            const auto obfFile = itObfFileEntry.value();

            obfFile->lockForRemoval();

            itObfFileEntry.remove();
            assert(obfFile.use_count() == 1);
        }

        // If all collected sources for current source origin are gone,
        // remove entire collection attached to source origin ID
        if(collectedSources.isEmpty())
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
        if(itCollectedSources == _collectedSources.end())
            itCollectedSources = _collectedSources.insert(originId, QHash<QString, std::shared_ptr<ObfFile> >());
        auto& collectedSources = *itCollectedSources;

        if(entry->type == SourceOriginType::Directory)
        {
            const auto& directoryAsSourceOrigin = std::static_pointer_cast<const DirectoryAsSourceOrigin>(entry);

            QFileInfoList obfFilesInfo;
            Utilities::findFiles(directoryAsSourceOrigin->directory, QStringList() << QLatin1String("*.obf"), obfFilesInfo, directoryAsSourceOrigin->isRecursive);
            for(const auto& obfFileInfo : constOf(obfFilesInfo))
            {
                const auto& obfFilePath = obfFileInfo.canonicalFilePath();
                if(collectedSources.constFind(obfFilePath) != collectedSources.cend())
                    continue;
                
                auto obfFile = new ObfFile(obfFilePath, obfFileInfo.size());
                collectedSources.insert(obfFilePath, std::shared_ptr<ObfFile>(obfFile));
            }

            if(directoryAsSourceOrigin->isRecursive)
            {
                QFileInfoList directoriesInfo;
                Utilities::findDirectories(directoryAsSourceOrigin->directory, QStringList() << QLatin1String("*"), directoriesInfo, true);

                for(const auto& directoryInfo : constOf(directoriesInfo))
                {
                    const auto canonicalPath = directoryInfo.canonicalFilePath();
                    if(directoryAsSourceOrigin->watchedSubdirectories.contains(canonicalPath))
                        continue;

                    _fileSystemWatcher->addPath(canonicalPath);
                    directoryAsSourceOrigin->watchedSubdirectories.insert(canonicalPath);
                }
            }
        }
        else if(entry->type == SourceOriginType::File)
        {
            const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(entry);

            if(!fileAsSourceOrigin->fileInfo.exists())
                continue;
            const auto& obfFilePath = fileAsSourceOrigin->fileInfo.canonicalFilePath();
            if(collectedSources.constFind(obfFilePath) != collectedSources.cend())
                continue;

            auto obfFile = new ObfFile(obfFilePath, fileAsSourceOrigin->fileInfo.size());
            collectedSources.insert(obfFilePath, std::shared_ptr<ObfFile>(obfFile));
        }
    }

    // Decrement invalidations counter with number of processed onces
    _collectedSourcesInvalidated.fetchAndAddOrdered(-invalidationsToProcess);

#if OSMAND_DEBUG
    const auto collectSources_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> collectSources_Elapsed = collectSources_End - collectSources_Begin;
    LogPrintf(LogSeverityLevel::Info, "Collected OBF sources in %fs", collectSources_Elapsed.count());
#endif
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
    if(recursive)
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
    if(itSourceOrigin == _sourcesOrigins.end())
        return false;

    const auto& sourceOrigin = *itSourceOrigin;
    if(sourceOrigin->type == SourceOriginType::Directory)
    {
        const auto& directoryAsSourceOrigin = std::static_pointer_cast<const DirectoryAsSourceOrigin>(sourceOrigin);

        for(const auto watchedSubdirectory : constOf(directoryAsSourceOrigin->watchedSubdirectories))
            _fileSystemWatcher->removePath(watchedSubdirectory);
        _fileSystemWatcher->removePath(directoryAsSourceOrigin->directory.canonicalPath());
    }
    else if(sourceOrigin->type == SourceOriginType::File)
    {
        const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(sourceOrigin);

        _fileSystemWatcher->removePath(fileAsSourceOrigin->fileInfo.canonicalFilePath());
    }

    _sourcesOrigins.erase(itSourceOrigin);

    invalidateCollectedSources();

    return true;
}

QVector< std::shared_ptr<const OsmAnd::ObfFile> > OsmAnd::ObfsCollection_P::getObfFiles() const
{
    // Check if sources were invalidated
    if(_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();

    QVector< std::shared_ptr<const OsmAnd::ObfFile> > obfFiles;
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

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection_P::obtainDataInterface() const
{
    // Check if sources were invalidated
    if(_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();

    // Create ObfReaders from collected sources
    QList< std::shared_ptr<const ObfReader> > obfReaders;
    {
        QReadLocker scopedLocker(&_collectedSourcesLock);

        for(const auto& collectedSources : constOf(_collectedSources))
        {
            obfReaders.reserve(obfReaders.size() + collectedSources.size());
            for(const auto& obfFile : constOf(collectedSources))
            {
                auto obfReader = new ObfReader(obfFile);
                obfReaders.push_back(qMove(std::shared_ptr<const ObfReader>(obfReader)));
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
