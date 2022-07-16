#include "TileSqliteDatabasesCollection_P.h"
#include "TileSqliteDatabasesCollection.h"

#include <cassert>

#include "QtCommon.h"

#include "OsmAndCore_private.h"
#include "QKeyValueIterator.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::TileSqliteDatabasesCollection_P::TileSqliteDatabasesCollection_P(
    TileSqliteDatabasesCollection* owner_,
    bool useFileWatcher /*= true*/,
    bool buildIndexes /*=true*/)
    : _fileSystemWatcher(useFileWatcher ? new QFileSystemWatcher() : NULL)
    , _lastUnusedSourceOriginId(0)
    , _buildIndexes(buildIndexes)
    , _collectedSourcesInvalidated(1)
    , owner(owner_)
{
    if (_fileSystemWatcher)
    {
        _fileSystemWatcher->moveToThread(gMainThread);

        _onDirectoryChangedConnection = QObject::connect(
            _fileSystemWatcher, &QFileSystemWatcher::directoryChanged,
            (std::function<void(const QString&)>)std::bind(&TileSqliteDatabasesCollection_P::onDirectoryChanged, this, std::placeholders::_1));
        _onFileChangedConnection = QObject::connect(
            _fileSystemWatcher, &QFileSystemWatcher::fileChanged,
            (std::function<void(const QString&)>)std::bind(&TileSqliteDatabasesCollection_P::onFileChanged, this, std::placeholders::_1));
    }
}

OsmAnd::TileSqliteDatabasesCollection_P::~TileSqliteDatabasesCollection_P()
{
    if (_fileSystemWatcher)
    {
        QObject::disconnect(_onDirectoryChangedConnection);
        QObject::disconnect(_onFileChangedConnection);

        _fileSystemWatcher->deleteLater();
    }
}

void OsmAnd::TileSqliteDatabasesCollection_P::invalidateCollectedSources()
{
    _collectedSourcesInvalidated.fetchAndAddOrdered(1);
}

void OsmAnd::TileSqliteDatabasesCollection_P::collectSources() const
{
    QWriteLocker scopedLocker1(&_collectedSourcesLock);
    QReadLocker scopedLocker2(&_sourcesOriginsLock);

    // Capture how many invalidations are going to be processed
    const auto invalidationsToProcess = _collectedSourcesInvalidated.loadAcquire();
    if (invalidationsToProcess == 0)
        return;

    const Stopwatch collectSourcesStopwatch(true);

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
                const auto database = itCollectedSource.value();

                if (_buildIndexes)
                    _collectedSourcesIndex.remove(database);

                itCollectedSource.value().reset();
            }

            itCollectedSourcesEntry.remove();
            continue;
        }

        // Check for missing files
        auto itDatabaseEntry = mutableIteratorOf(collectedSources);
        while(itDatabaseEntry.hasNext())
        {
            const auto& sourceFilename = itDatabaseEntry.next().key();
            if (QFile::exists(sourceFilename))
                continue;
            const auto database = itDatabaseEntry.value();

            if (_buildIndexes)
                _collectedSourcesIndex.remove(database);

            itDatabaseEntry.remove();
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
            itCollectedSources = _collectedSources.insert(originId, QHash<QString, std::shared_ptr<TileSqliteDatabase> >());
        auto& collectedSources = *itCollectedSources;

        if (entry->type == SourceOriginType::Directory)
        {
            const auto& directoryAsSourceOrigin = std::static_pointer_cast<const DirectoryAsSourceOrigin>(entry);

            QFileInfoList sqliteFilesInfo;
            Utilities::findFiles(
                directoryAsSourceOrigin->directory,
                QStringList() << QStringLiteral("*.sqlite"),
                sqliteFilesInfo,
                directoryAsSourceOrigin->isRecursive
            );
            for(const auto& sqliteFileInfo : constOf(sqliteFilesInfo))
            {
                const auto& sqliteFilePath = sqliteFileInfo.canonicalFilePath();
                if (_buildIndexes)
                {
                    const auto citDatabase = collectedSources.constFind(sqliteFilePath);
                    if (citDatabase != collectedSources.cend())
                    {
                        _collectedSourcesIndex.refresh(*citDatabase);
                    }
                }
                
                const auto database = std::make_shared<TileSqliteDatabase>(sqliteFilePath);
                collectedSources.insert(sqliteFilePath, database);

                if (_buildIndexes)
                    _collectedSourcesIndex.add(database);
            }

            if (_fileSystemWatcher && directoryAsSourceOrigin->isRecursive)
            {
                QFileInfoList directoriesInfo;
                Utilities::findDirectories(
                    directoryAsSourceOrigin->directory,
                    QStringList() << QStringLiteral("*"),
                    directoriesInfo,
                    true
                );

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
            const auto& sqliteFilePath = fileAsSourceOrigin->fileInfo.canonicalFilePath();
            const auto citDatabase = collectedSources.constFind(sqliteFilePath);
            if (citDatabase != collectedSources.cend())
            {
                if (_buildIndexes)
                    _collectedSourcesIndex.refresh(*citDatabase);
                
                continue;
            }

            const auto database = std::make_shared<TileSqliteDatabase>(sqliteFilePath);
            collectedSources.insert(sqliteFilePath, database);

            if (_buildIndexes)
                _collectedSourcesIndex.add(database);
        }
    }

    // Decrement invalidations counter with number of processed onces
    _collectedSourcesInvalidated.fetchAndAddOrdered(-invalidationsToProcess);

    LogPrintf(LogSeverityLevel::Info, "Collected tile SQLite sources in %fs", collectSourcesStopwatch.elapsed());
}

QList<OsmAnd::TileSqliteDatabasesCollection::SourceOriginId> OsmAnd::TileSqliteDatabasesCollection_P::getSourceOriginIds() const
{
    QReadLocker scopedLocker(&_sourcesOriginsLock);

    return _sourcesOrigins.keys();
}

OsmAnd::TileSqliteDatabasesCollection::SourceOriginId OsmAnd::TileSqliteDatabasesCollection_P::addDirectory(
    const QDir& dir,
    bool recursive)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto allocatedId = _lastUnusedSourceOriginId++;
    auto sourceOrigin = new DirectoryAsSourceOrigin();
    sourceOrigin->directory = dir;
    sourceOrigin->isRecursive = recursive;
    _sourcesOrigins.insert(allocatedId, qMove(std::shared_ptr<const SourceOrigin>(sourceOrigin)));

    if (_fileSystemWatcher)
    {
        _fileSystemWatcher->addPath(dir.canonicalPath());
        if (recursive)
        {
            QFileInfoList subdirs;
            Utilities::findDirectories(dir, QStringList() << QStringLiteral("*"), subdirs, true);
            for(const auto& subdir : subdirs)
            {
                const auto canonicalPath = subdir.canonicalFilePath();
                sourceOrigin->watchedSubdirectories.insert(canonicalPath);
                _fileSystemWatcher->addPath(canonicalPath);
            }
        }
    }

    invalidateCollectedSources();

    return allocatedId;
}

OsmAnd::TileSqliteDatabasesCollection::SourceOriginId OsmAnd::TileSqliteDatabasesCollection_P::addFile(
    const QFileInfo& fileInfo)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto allocatedId = _lastUnusedSourceOriginId++;
    auto sourceOrigin = new TileSqliteDatabasesCollection_P::FileAsSourceOrigin();
    sourceOrigin->fileInfo = fileInfo;
    _sourcesOrigins.insert(allocatedId, qMove(std::shared_ptr<const SourceOrigin>(sourceOrigin)));

    if (_fileSystemWatcher)
        _fileSystemWatcher->addPath(fileInfo.canonicalFilePath());

    invalidateCollectedSources();

    return allocatedId;
}

bool OsmAnd::TileSqliteDatabasesCollection_P::removeFile(const QFileInfo& fileInfo)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    for(const auto& itEntry : rangeOf(constOf(_sourcesOrigins)))
    {
        const auto& originId = itEntry.key();
        const auto& entry = itEntry.value();
        
        if (entry->type == SourceOriginType::File)
        {
            const auto& fileAsSourceOrigin = std::static_pointer_cast<const FileAsSourceOrigin>(entry);
            if (fileAsSourceOrigin->fileInfo == fileInfo)
            {
                if (_fileSystemWatcher)
                    _fileSystemWatcher->removePath(fileAsSourceOrigin->fileInfo.canonicalFilePath());
                
                _sourcesOrigins.remove(originId);

                invalidateCollectedSources();
                return true;
            }
        }
    }
    return false;
}

bool OsmAnd::TileSqliteDatabasesCollection_P::remove(const TileSqliteDatabasesCollection::SourceOriginId entryId)
{
    QWriteLocker scopedLocker(&_sourcesOriginsLock);

    const auto itSourceOrigin = _sourcesOrigins.find(entryId);
    if (itSourceOrigin == _sourcesOrigins.end())
        return false;

    if (_fileSystemWatcher)
    {
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
    }
    
    _sourcesOrigins.erase(itSourceOrigin);

    invalidateCollectedSources();
    return true;
}

QList< std::shared_ptr<const OsmAnd::TileSqliteDatabase> > OsmAnd::TileSqliteDatabasesCollection_P::getTileSqliteDatabases() const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();

    QList< std::shared_ptr<const OsmAnd::TileSqliteDatabase> > databases;
    {
        QReadLocker scopedLocker(&_collectedSourcesLock);

        for(const auto& collectedSources : constOf(_collectedSources))
        {
            databases.reserve(databases.size() + collectedSources.size());
            for(const auto& database : collectedSources)
                databases.append(database);
        }
    }
    return databases;
}

QList< std::shared_ptr<const OsmAnd::TileSqliteDatabase> > OsmAnd::TileSqliteDatabasesCollection_P::getTileSqliteDatabases(
    TileId tileId, ZoomLevel zoom) const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();

    QList< std::shared_ptr<const OsmAnd::TileSqliteDatabase> > databases;
    {
        QReadLocker scopedLocker(&_collectedSourcesLock);

        if (_buildIndexes)
            databases = constOf(_collectedSourcesIndex).find(tileId, zoom);
    }
    return databases;
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::TileSqliteDatabasesCollection_P::getTileSqliteDatabase(const QFileInfo& fileInfo) const
{
    // Check if sources were invalidated
    if (_collectedSourcesInvalidated.loadAcquire() > 0)
        collectSources();
    
    std::shared_ptr<const OsmAnd::TileSqliteDatabase> database;
    {
        QReadLocker scopedLocker(&_collectedSourcesLock);
        
        for (const auto& collectedSources : constOf(_collectedSources))
        {
            const auto& filePath = fileInfo.canonicalFilePath();
            const auto citDatabase = collectedSources.constFind(filePath);
            if (citDatabase != collectedSources.cend())
                return *citDatabase;
        }
    }
    return nullptr;
}

void OsmAnd::TileSqliteDatabasesCollection_P::onDirectoryChanged(const QString& path)
{
    invalidateCollectedSources();
}

void OsmAnd::TileSqliteDatabasesCollection_P::onFileChanged(const QString& path)
{
    invalidateCollectedSources();
}
