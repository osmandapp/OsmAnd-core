#include "TileSqliteDatabasesIndex.h"

#include "TileSqliteDatabasesIndex_P.h"

OsmAnd::TileSqliteDatabasesIndex::TileSqliteDatabasesIndex()
    : _p(new TileSqliteDatabasesIndex_P(this))
{
}

OsmAnd::TileSqliteDatabasesIndex::~TileSqliteDatabasesIndex()
{
}

bool OsmAnd::TileSqliteDatabasesIndex::add(const std::shared_ptr<TileSqliteDatabase>& database)
{
    return _p->add(database);
}

bool OsmAnd::TileSqliteDatabasesIndex::remove(const std::shared_ptr<TileSqliteDatabase>& database)
{
    return _p->remove(database);
}

bool OsmAnd::TileSqliteDatabasesIndex::removeAll()
{
    return _p->removeAll();
}

QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex::getAll()
{
    return _p->getAll();
}

QList<std::shared_ptr<const OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex::getAll() const
{
    return _p->getAll();
}

bool OsmAnd::TileSqliteDatabasesIndex::shouldRefresh(const std::shared_ptr<TileSqliteDatabase>& database)
{
    return _p->shouldRefresh(database);
}

bool OsmAnd::TileSqliteDatabasesIndex::refresh(const std::shared_ptr<TileSqliteDatabase>& database, bool force /* = false */)
{
    return _p->refresh(database, force);
}

bool OsmAnd::TileSqliteDatabasesIndex::refreshAll(bool force /* = false */)
{
    return _p->refreshAll(force);
}

QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex::find(TileId tileId, ZoomLevel zoom)
{
    return _p->find(tileId, zoom);
}

QList<std::shared_ptr<const OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex::find(TileId tileId, ZoomLevel zoom) const
{
    return _p->find(tileId, zoom);
}

OsmAnd::TileSqliteDatabasesIndex::Info::Info(
    QDateTime timestamp_ /* = QDateTime::currentDateTimeUtc() */,
    TileSqliteDatabase::Meta meta_ /* = TileSqliteDatabase::Meta() */)
    : timestamp(timestamp_)
    , meta(meta_)
{
}

OsmAnd::TileSqliteDatabasesIndex::Info::~Info()
{
}
