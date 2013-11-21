#include "ObfsCollection_P.h"
#include "ObfsCollection.h"

#include <chrono>

#include "ObfReader.h"
#include "ObfDataInterface.h"
#include "ObfFile.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::ObfsCollection_P::ObfsCollection_P( ObfsCollection* owner_ )
    : owner(owner_)
    , _watchedCollectionMutex(QMutex::Recursive)
    , _watchedCollectionChanged(false)
    , _sourcesMutex(QMutex::Recursive)
    , _sourcesRefreshedOnce(false)
{
}

OsmAnd::ObfsCollection_P::~ObfsCollection_P()
{
}

void OsmAnd::ObfsCollection_P::refreshSources()
{
#if defined(_DEBUG) || defined(DEBUG)
    const auto refreshSources_Begin = std::chrono::high_resolution_clock::now();
#endif

    QMutexLocker scopedLock(&_sourcesMutex);

    // Find all files that are present in watched entries
    QFileInfoList obfs;
    {
        QMutexLocker scopedLock(&_watchedCollectionMutex);

        for(auto itEntry = _watchedCollection.cbegin(); itEntry != _watchedCollection.cend(); ++itEntry)
        {
            const auto& entry = *itEntry;

            if(entry->type == WatchEntry::WatchedDirectory)
            {
                const auto& watchedDirEntry = std::static_pointer_cast<WatchedDirectoryEntry>(entry);

                Utilities::findFiles(watchedDirEntry->dir, QStringList() << QLatin1String("*.obf"), obfs, watchedDirEntry->recursive);
            }
            else if(entry->type == WatchEntry::ExplicitFile)
            {
                const auto& explicitFileEntry = std::static_pointer_cast<ExplicitFileEntry>(entry);

                if(explicitFileEntry->fileInfo.exists())
                    obfs.push_back(explicitFileEntry->fileInfo);
            }
        }
    }

    // For each file in registry, ...
    {
        QMutableHashIterator< QString, std::shared_ptr<ObfFile> > itObfFileEntry(_sources);
        while(itObfFileEntry.hasNext())
        {
            itObfFileEntry.next();

            // ... which does not exist, ...
            if(QFile::exists(itObfFileEntry.key()))
                continue;

            // ... remove entry
            itObfFileEntry.remove();
        }
    }

    // For each file, ...
    for(auto itObfFileInfo = obfs.cbegin(); itObfFileInfo != obfs.cend(); ++itObfFileInfo)
    {
        const auto& obfFileInfo = *itObfFileInfo;

        const auto& obfFilePath = obfFileInfo.canonicalFilePath();
        auto itObfFileEntry = _sources.constFind(obfFilePath);

        // ... which is not yet present in registry, ...
        if(itObfFileEntry == _sources.cend())
        {
            // ... create ObfFile
            auto obfFile = new ObfFile(obfFilePath);
            itObfFileEntry = _sources.insert(obfFilePath, std::shared_ptr<ObfFile>(obfFile));
        }
    }

#if defined(_DEBUG) || defined(DEBUG)
    const auto refreshSources_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> refreshSources_Elapsed = refreshSources_End - refreshSources_Begin;
    LogPrintf(LogSeverityLevel::Info, "Refreshed OBF sources in %fs", refreshSources_Elapsed.count());
#endif

    // Mark that sources were refreshed at least once
    _sourcesRefreshedOnce = true;
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection_P::obtainDataInterface()
{
    QMutexLocker scopedLock_sourcesMutex(&_sourcesMutex);
    QMutexLocker scopedLock_watchedEntries(&_watchedCollectionMutex);

    // Refresh sources
    if(!_sourcesRefreshedOnce)
    {
#if defined(DEBUG) || defined(_DEBUG)
        LogPrintf(LogSeverityLevel::Info, "Refreshing OBF sources because they were never initialized");
#endif

        // if sources have never been initialized
        refreshSources();

        // Clear changed flag if it's raised, since it was already processed
        if(_watchedCollectionChanged)
            _watchedCollectionChanged = false;
    }
    else if(_watchedCollectionChanged)
    {
#if defined(DEBUG) || defined(_DEBUG)
        LogPrintf(LogSeverityLevel::Info, "Refreshing OBF sources because watch-collecting was changed");
#endif

        // if watched collection has changed
        refreshSources();
        _watchedCollectionChanged = false;
    }

    QList< std::shared_ptr<ObfReader> > obfReaders;
    for(auto itSource = _sources.cbegin(); itSource != _sources.cend(); ++itSource)
    {
        const auto& obfFile = itSource.value();

        auto obfReader = new ObfReader(obfFile);
        obfReaders.push_back(qMove(std::shared_ptr<ObfReader>(obfReader)));
    }

    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface(obfReaders));
}
