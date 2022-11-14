#include "TileSqliteDatabasesCollection.h"
#include "TileSqliteDatabasesCollection_P.h"

OsmAnd::TileSqliteDatabasesCollection::TileSqliteDatabasesCollection(bool useFileWatcher /*= true*/, bool buildIndexes /*= true*/)
    : _p(new TileSqliteDatabasesCollection_P(this, useFileWatcher, buildIndexes))
{
}

OsmAnd::TileSqliteDatabasesCollection::~TileSqliteDatabasesCollection()
{
}

QList<OsmAnd::TileSqliteDatabasesCollection::SourceOriginId> OsmAnd::TileSqliteDatabasesCollection::getSourceOriginIds() const
{
    return _p->getSourceOriginIds();
}

OsmAnd::TileSqliteDatabasesCollection::SourceOriginId OsmAnd::TileSqliteDatabasesCollection::addDirectory(
    const QString& dirPath,
    bool recursive /*= true*/)
{
    return addDirectory(QDir(dirPath), recursive);
}

OsmAnd::TileSqliteDatabasesCollection::SourceOriginId OsmAnd::TileSqliteDatabasesCollection::addDirectory(
    const QDir& dir,
    bool recursive /*= true*/)
{
    return _p->addDirectory(dir, recursive);
}

OsmAnd::TileSqliteDatabasesCollection::SourceOriginId OsmAnd::TileSqliteDatabasesCollection::addFile(
    const QString& filePath)
{
    return addFile(QFileInfo(filePath));
}

OsmAnd::TileSqliteDatabasesCollection::SourceOriginId OsmAnd::TileSqliteDatabasesCollection::addFile(
    const QFileInfo& fileInfo)
{
    return _p->addFile(fileInfo);
}

bool OsmAnd::TileSqliteDatabasesCollection::removeFile(const QString& filePath)
{
    return _p->removeFile(QFileInfo(filePath));
}

bool OsmAnd::TileSqliteDatabasesCollection::remove(const SourceOriginId entryId)
{
    return _p->remove(entryId);
}

OsmAnd::ZoomLevel OsmAnd::TileSqliteDatabasesCollection::getMinZoom() const
{
    return _p->getMinZoom();
}

OsmAnd::ZoomLevel OsmAnd::TileSqliteDatabasesCollection::getMaxZoom() const
{
    return _p->getMaxZoom();
}

QList< std::shared_ptr<const OsmAnd::TileSqliteDatabase> >OsmAnd::TileSqliteDatabasesCollection::getTileSqliteDatabases() const
{
    return _p->getTileSqliteDatabases();
}

QList< std::shared_ptr<const OsmAnd::TileSqliteDatabase> >OsmAnd::TileSqliteDatabasesCollection::getTileSqliteDatabases(
    TileId tileId,
    ZoomLevel zoom) const
{
    return _p->getTileSqliteDatabases(tileId, zoom);
}

std::shared_ptr<OsmAnd::TileSqliteDatabase> OsmAnd::TileSqliteDatabasesCollection::getTileSqliteDatabase(const QString& filePath) const
{
    return _p->getTileSqliteDatabase(QFileInfo(filePath));
}

