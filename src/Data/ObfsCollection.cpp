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
    _d->_watchedCollection.push_back(qMove(std::shared_ptr<ObfsCollection_P::WatchEntry>(entry)));
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
    _d->_watchedCollection.push_back(qMove(std::shared_ptr<ObfsCollection_P::WatchEntry>(entry)));
    _d->_watchedCollectionChanged = true;
}

void OsmAnd::ObfsCollection::registerExplicitFile( const QString& filePath )
{
    registerExplicitFile(QFileInfo(filePath));
}

std::shared_ptr<OsmAnd::ObfDataInterface> OsmAnd::ObfsCollection::obtainDataInterface() const
{
    return _d->obtainDataInterface();
}
