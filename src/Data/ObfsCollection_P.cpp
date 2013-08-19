#include "ObfsCollection_P.h"
#include "ObfsCollection.h"

#include "ObfFile.h"
#include "Utilities.h"

OsmAnd::ObfsCollection_P::ObfsCollection_P( ObfsCollection* owner_ )
    : owner(owner_)
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

    // Find all files that are present if data path
    QFileInfoList obfs;
    Utilities::findFiles(owner->dataDir, QStringList() << "*.obf", obfs, true);

    // For each file in registry, ...
    {
        QMutableHashIterator< QString, std::shared_ptr<ObfFile> > itObfFileEntry(_sources);
        while(itObfFileEntry.hasNext())
        {
            // ... which does not exist, ...
            if(QFile::exists(itObfFileEntry.key()))
            {
                itObfFileEntry.next();
                continue;
            }

            // ... remove entry
            itObfFileEntry.remove();
        }
    }

    // For each file, ...
    for(auto itObfFileInfo = obfs.begin(); itObfFileInfo != obfs.end(); ++itObfFileInfo)
    {
        const auto& obfFileInfo = *itObfFileInfo;

        const auto& obfFilePath = obfFileInfo.canonicalFilePath();
        auto itObfFileEntry = _sources.find(obfFilePath);

        // ... which is not yet present in registry, ...
        if(itObfFileEntry == _sources.end())
        {
            // ... create ObfFile
            auto obfFile = new ObfFile(obfFilePath);
            itObfFileEntry = _sources.insert(obfFilePath, std::shared_ptr<ObfFile>(obfFile));
        }
    }

    // Mark that sources were refreshed at least once
    _sourcesRefreshedOnce = true;
}
