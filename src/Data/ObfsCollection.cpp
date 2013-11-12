#include "ObfsCollection.h"
#include "ObfsCollection_P.h"

#include "ObfReader.h"
#include "ObfDataInterface.h"
#include "Logging.h"

OsmAnd::ObfsCollection::ObfsCollection()
    : _d(new ObfsCollection_P(this))
{
}

OsmAnd::ObfsCollection::~ObfsCollection()
{
}

void OsmAnd::ObfsCollection::watchDirectory( const QDir& dir, bool recursive /*= true*/ )
{
    QMutexLocker scopedLock(&_d->_watchedCollectionMutex);

    auto entry = new ObfsCollection_P::WatchedDirectoryEntry();
    entry->dir = dir;
    entry->recursive = recursive;
    _d->_watchedCollection.push_back(std::shared_ptr<ObfsCollection_P::WatchEntry>(entry));
    _d->_watchedCollectionChanged = true;
}

void OsmAnd::ObfsCollection::watchDirectory( const QString& dirPath, bool recursive /*= true*/ )
{
    watchDirectory(QDir(dirPath), recursive);
}

void OsmAnd::ObfsCollection::registerExplicitFile( const QFileInfo& fileInfo )
{
    QMutexLocker scopedLock(&_d->_watchedCollectionMutex);

    auto entry = new ObfsCollection_P::ExplicitFileEntry();
    entry->fileInfo = fileInfo;
    _d->_watchedCollection.push_back(std::shared_ptr<ObfsCollection_P::WatchEntry>(entry));
    _d->_watchedCollectionChanged = true;
}

void OsmAnd::ObfsCollection::registerExplicitFile( const QString& filePath )
{
    registerExplicitFile(QFileInfo(filePath));
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface() const
{
    QMutexLocker scopedLock_sourcesMutex(&_d->_sourcesMutex);
    QMutexLocker scopedLock_watchedEntries(&_d->_watchedCollectionMutex);

    // Refresh sources
    if(!_d->_sourcesRefreshedOnce)
    {
#if defined(DEBUG) || defined(_DEBUG)
        LogPrintf(LogSeverityLevel::Info, "Refreshing OBF sources because they were never initialized");
#endif

        // if sources have never been initialized
        _d->refreshSources();
    }
    else if(_d->_watchedCollectionChanged)
    {
#if defined(DEBUG) || defined(_DEBUG)
        LogPrintf(LogSeverityLevel::Info, "Refreshing OBF sources because watch-collecting was changed");
#endif

        // if watched collection has changed
        _d->refreshSources();
        _d->_watchedCollectionChanged = false;
    }

    QList< std::shared_ptr<ObfReader> > obfReaders;
    for(auto itSource = _d->_sources.cbegin(); itSource != _d->_sources.cend(); ++itSource)
    {
        const auto& obfFile = itSource.value();

        auto obfReader = new ObfReader(obfFile);
        obfReaders.push_back(std::shared_ptr<ObfReader>(obfReader));
    }

    return std::shared_ptr<ObfDataInterface>(new ObfDataInterface(obfReaders));
}
