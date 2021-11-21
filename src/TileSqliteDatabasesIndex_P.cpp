#include "TileSqliteDatabasesIndex_P.h"

#include "ignore_warnings_on_external_includes.h"
#include <QFileInfo>
#include "restore_internal_warnings.h"

#include "TileSqliteDatabasesIndex.h"
#include "Utilities.h"

OsmAnd::TileSqliteDatabasesIndex_P::TileSqliteDatabasesIndex_P(TileSqliteDatabasesIndex* owner_)
    : _tree(AreaI::largestPositive())
    , owner(owner_)
{
}

OsmAnd::TileSqliteDatabasesIndex_P::~TileSqliteDatabasesIndex_P()
{
}

bool OsmAnd::TileSqliteDatabasesIndex_P::shouldRefresh(
    const std::shared_ptr<TileSqliteDatabase>& database,
    const Info& info) const
{
    auto timestamp = QDateTime::currentDateTimeUtc();
    if (!database->filename.isEmpty())
    {
        timestamp = QFileInfo(database->filename).lastModified().toUTC();
    }

    return timestamp > info.timestamp;
}

bool OsmAnd::TileSqliteDatabasesIndex_P::add(const std::shared_ptr<TileSqliteDatabase>& database)
{
    if (!database->open())
    {
        return false;
    }

    TileSqliteDatabase::Meta meta;
    if (!database->obtainMeta(meta))
    {
        return false;
    }

    AreaI bbox31;
    if (!database->recomputeBBox31(&bbox31))
    {
        return false;
    }

    {
        QWriteLocker scopedLocker(&_lock);

        if (!_tree.insert(database, bbox31))
        {
            return false;
        }
        _infos[database] = Info(QDateTime::currentDateTimeUtc(), meta);
    }

    return true;
}

bool OsmAnd::TileSqliteDatabasesIndex_P::remove(const std::shared_ptr<TileSqliteDatabase>& database)
{
    QWriteLocker scopedLocker(&_lock);

    if (!_tree.removeOne(database, database->getBBox31()))
    {
        if (!_tree.removeOneSlow(database))
        {
            return false;
        }
    }

    _infos.remove(database);

    return true;
}

bool OsmAnd::TileSqliteDatabasesIndex_P::removeAll()
{
    QWriteLocker scopedLocker(&_lock);

    _tree.clear();
    _infos.clear();

    return true;
}

QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex_P::getAll()
{
    QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> result;
    {
        QReadLocker scopedLocker(&_lock);

        _tree.get(result);
    }
    return result;
}

QList<std::shared_ptr<const OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex_P::getAll() const
{
    QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> result;
    {
        QReadLocker scopedLocker(&_lock);

        _tree.get(result);
    }
    return copyAs< QList<std::shared_ptr<const OsmAnd::TileSqliteDatabase>> >(result);
}

bool OsmAnd::TileSqliteDatabasesIndex_P::shouldRefresh(const std::shared_ptr<TileSqliteDatabase>& database) const
{
    QReadLocker scopedLocker(&_lock);

    const auto citInfo = _infos.constFind(database);
    if (citInfo == _infos.cend())
    {
        return false;
    }

    return shouldRefresh(database, *citInfo);
}

bool OsmAnd::TileSqliteDatabasesIndex_P::refresh(
    const std::shared_ptr<TileSqliteDatabase>& database,
    bool force /* = false */)
{
    if (!force && !shouldRefresh(database))
    {
        return false;
    }

    if (!database->open())
    {
        return false;
    }

    TileSqliteDatabase::Meta meta;
    if (!database->obtainMeta(meta))
    {
        return false;
    }

    const auto currentBBox31 = database->getBBox31();
    AreaI newBBox31;
    if (!database->recomputeBBox31(&newBBox31))
    {
        return false;
    }

    {
        QWriteLocker scopedLocker(&_lock);

        if (!_tree.removeOne(database, currentBBox31))
        {
            if (!_tree.removeOneSlow(database))
            {
                return false;
            }
        }

        if (!_tree.insert(database, newBBox31))
        {
            return false;
        }

        _infos[database] = Info(QDateTime::currentDateTimeUtc(), meta);
    }

    return true;
}

bool OsmAnd::TileSqliteDatabasesIndex_P::refreshAll(bool force /* = false */)
{
    QWriteLocker scopedLocker(&_lock);

    _tree.clear();
    auto itInfoEntry = mutableIteratorOf(_infos);
    while (itInfoEntry.hasNext())
    {
        const auto& infoEntry = itInfoEntry.next();
        const auto& database = infoEntry.key();
        const auto& info = infoEntry.value();

        if (!database->open())
        {
            itInfoEntry.remove();
            continue;
        }

        AreaI bbox31;
        if (force || shouldRefresh(database, info))
        {
            if (!database->recomputeBBox31(&bbox31))
            {
                itInfoEntry.remove();
            }
        }
        else
        {
            bbox31 = database->getBBox31();
        }

        TileSqliteDatabase::Meta meta;
        if (!database->obtainMeta(meta))
        {
            itInfoEntry.remove();
            continue;
        }

        if (!_tree.insert(database, bbox31))
        {
            itInfoEntry.remove();
            continue;
        }

        _infos[database] = Info(QDateTime::currentDateTimeUtc(), meta);
    }

    return true;
}

QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex_P::find(
    TileId tileId,
    ZoomLevel zoom)
{
    const auto bbox31 = Utilities::tileBoundingBox31(tileId, zoom);

    QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> result;
    {
        QReadLocker scopedLocker(&_lock);

        _tree.query(bbox31, result);
    }

    return result;
}

QList<std::shared_ptr<const OsmAnd::TileSqliteDatabase>> OsmAnd::TileSqliteDatabasesIndex_P::find(
    TileId tileId,
    ZoomLevel zoom) const
{
    const auto bbox31 = Utilities::tileBoundingBox31(tileId, zoom);

    QList<std::shared_ptr<OsmAnd::TileSqliteDatabase>> result;
    {
        QReadLocker scopedLocker(&_lock);

        _tree.query(bbox31, result);
    }

    return copyAs< QList<std::shared_ptr<const OsmAnd::TileSqliteDatabase>> >(result);
}
