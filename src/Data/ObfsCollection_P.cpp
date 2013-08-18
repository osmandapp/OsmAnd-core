#include "ObfsCollection_P.h"
#include "ObfsCollection.h"

#include "ObfReader.h"
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
    Utilities::findFiles(owner->dataPath, QStringList() << "*.obf", obfs, true);

    // For each file, which is not yet opened, create OBF reader
    for(auto itObfFileInfo = obfs.begin(); itObfFileInfo != obfs.end(); ++itObfFileInfo)
    {
        const auto& obfFileInfo = *itObfFileInfo;

        const auto& obfFilePath = obfFileInfo.absoluteFilePath();
        auto itObfReader = _sources.find(obfFilePath);
        if(itObfReader == _sources.end())
        {
            //auto obfReader = new ObfReader(obfFilePath);
            //itObfReader = _sources.insert(obfFilePath, std::shared_ptr<ObfReader>(obfReader));
        }
    }

    // Mark that sources were refreshed at least once
    _sourcesRefreshedOnce = true;
}
