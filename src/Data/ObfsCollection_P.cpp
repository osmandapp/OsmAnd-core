#include "ObfsCollection_P.h"
#include "ObfsCollection.h"

#include "ObfFile.h"
#include "Utilities.h"

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

    // Mark that sources were refreshed at least once
    _sourcesRefreshedOnce = true;
}
