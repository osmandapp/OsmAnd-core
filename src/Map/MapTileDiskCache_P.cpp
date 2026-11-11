#include "MapTileDiskCache_P.h"
#include "MapTileDiskCache.h"

#include "ignore_warnings_on_external_includes.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include "restore_internal_warnings.h"

#include "Logging.h"

namespace
{
    const char* const kCreateSql =
        "CREATE TABLE IF NOT EXISTS environments ("
        "  hash INTEGER PRIMARY KEY,"
        "  used INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS maps ("
        "  path TEXT PRIMARY KEY,"
        "  size INTEGER NOT NULL,"
        "  timestamp INTEGER NOT NULL,"
        "  top INTEGER NOT NULL, left INTEGER NOT NULL, bottom INTEGER NOT NULL, right INTEGER NOT NULL);"
        "CREATE TABLE IF NOT EXISTS tiles ("
        "  environment INTEGER NOT NULL,"
        "  kind INTEGER NOT NULL,"
        "  zoom INTEGER NOT NULL,"
        "  x INTEGER NOT NULL,"
        "  y INTEGER NOT NULL,"
        "  variant INTEGER NOT NULL,"
        "  atime INTEGER NOT NULL,"
        "  weight INTEGER NOT NULL,"
        "  size INTEGER NOT NULL,"
        "  data BLOB NOT NULL,"
        "  UNIQUE (environment, kind, zoom, x, y, variant));";

    const char* const kKeyWhere =
        " WHERE environment = ?1 AND kind = ?2 AND zoom = ?3 AND x = ?4 AND y = ?5 AND variant = ?6";

    inline int64_t asSigned(const uint64_t v)
    {
        return static_cast<int64_t>(v);
    }
}

OsmAnd::MapTileDiskCache_P::MapTileDiskCache_P(MapTileDiskCache* const owner_, const int64_t limitBytes)
    : _db(nullptr)
    , _getStmt(nullptr)
    , _containsStmt(nullptr)
    , _insertStmt(nullptr)
    , _sizeStmt(nullptr)
    , _touchStmt(nullptr)
    , _limit(limitBytes)
    , _keptEnvironments(MapTileDiskCache::DefaultKeptEnvironments)
    , _totalBytes(0)
    , _entriesCount(0)
    , owner(owner_)
{
}

OsmAnd::MapTileDiskCache_P::~MapTileDiskCache_P()
{
    QMutexLocker scopedLocker(&_mutex);
    closeLocked();
}

int64_t OsmAnd::MapTileDiskCache_P::now() const
{
    return _clock ? _clock() : QDateTime::currentMSecsSinceEpoch() / 1000;
}

bool OsmAnd::MapTileDiskCache_P::exec(const char* sql)
{
    if (!_db)
        return false;
    char* error = nullptr;
    const auto rc = sqlite3_exec(_db, sql, nullptr, nullptr, &error);
    if (rc != SQLITE_OK)
    {
        LogPrintf(LogSeverityLevel::Warning, "MapTileDiskCache: '%s' failed: %s", sql, error ? error : "?");
        sqlite3_free(error);
        checkError(rc);
        return false;
    }
    return true;
}

void OsmAnd::MapTileDiskCache_P::checkError(const int rc)
{
    // a damaged file is not worth repairing: start from scratch
    const auto primary = rc & 0xFF;
    if (primary == SQLITE_CORRUPT || primary == SQLITE_NOTADB)
    {
        LogPrintf(LogSeverityLevel::Warning, "MapTileDiskCache: '%s' is damaged (%d), creating it again",
            qPrintable(owner->filePath), rc);
        recreateLocked();
    }
}

bool OsmAnd::MapTileDiskCache_P::open()
{
    QMutexLocker scopedLocker(&_mutex);
    return openLocked(true);
}

bool OsmAnd::MapTileDiskCache_P::openLocked(const bool allowRecreate)
{
    closeLocked();
    QDir().mkpath(QFileInfo(owner->filePath).absolutePath());
    auto rc = sqlite3_open_v2(owner->filePath.toUtf8().constData(), &_db,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr);
    if (rc != SQLITE_OK)
    {
        LogPrintf(LogSeverityLevel::Warning, "MapTileDiskCache: can not open '%s': %d", qPrintable(owner->filePath), rc);
        if (_db)
            sqlite3_close(_db);
        _db = nullptr;
        if (allowRecreate && QFile::exists(owner->filePath) && QFile::remove(owner->filePath))
            return openLocked(false);
        return false;
    }

    int version = -1;
    sqlite3_stmt* stmt = nullptr;
    rc = sqlite3_prepare_v2(_db, "PRAGMA user_version", -1, &stmt, nullptr);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        version = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    int tables = -1;
    rc = sqlite3_prepare_v2(_db, "SELECT count(*) FROM sqlite_master", -1, &stmt, nullptr);
    if (rc == SQLITE_OK && sqlite3_step(stmt) == SQLITE_ROW)
        tables = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    const bool fresh = tables == 0;
    if (!fresh && version != SchemaVersion)
    {
        // another format or not our file at all
        closeLocked();
        if (allowRecreate && QFile::remove(owner->filePath))
            return openLocked(false);
        return false;
    }

    // auto_vacuum only takes effect before the first table is created
    if (fresh && !exec("PRAGMA auto_vacuum = INCREMENTAL"))
    {
        closeLocked();
        return false;
    }
    // it is a cache: losing the last writes on a power cut is fine, a damaged file is detected and dropped
    exec("PRAGMA journal_mode = TRUNCATE");
    exec("PRAGMA synchronous = OFF");
    if (!exec(kCreateSql))
    {
        closeLocked();
        if (allowRecreate && QFile::remove(owner->filePath))
            return openLocked(false);
        return false;
    }
    if (fresh)
        exec(qPrintable(QStringLiteral("PRAGMA user_version = %1").arg(SchemaVersion)));

    const auto prepare =
        [this]
        (const QString& sql, sqlite3_stmt** outStmt) -> bool
        {
            return sqlite3_prepare_v2(_db, sql.toUtf8().constData(), -1, outStmt, nullptr) == SQLITE_OK;
        };
    if (!prepare(QStringLiteral("SELECT rowid, atime, data FROM tiles") + kKeyWhere, &_getStmt)
        || !prepare(QStringLiteral("SELECT 1 FROM tiles") + kKeyWhere, &_containsStmt)
        || !prepare(QStringLiteral("SELECT size FROM tiles") + kKeyWhere, &_sizeStmt)
        || !prepare(QStringLiteral("INSERT OR REPLACE INTO tiles (environment, kind, zoom, x, y, variant, atime, weight, size, data) "
            "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10)"), &_insertStmt)
        || !prepare(QStringLiteral("UPDATE tiles SET atime = ?1 WHERE rowid = ?2"), &_touchStmt))
    {
        LogPrintf(LogSeverityLevel::Warning, "MapTileDiskCache: can not prepare statements: %s", sqlite3_errmsg(_db));
        closeLocked();
        return false;
    }

    recountLocked();
    loadEnvironmentsLocked();
    return true;
}

void OsmAnd::MapTileDiskCache_P::closeLocked()
{
    for (auto stmt : { &_getStmt, &_containsStmt, &_insertStmt, &_sizeStmt, &_touchStmt })
    {
        if (*stmt)
            sqlite3_finalize(*stmt);
        *stmt = nullptr;
    }
    if (_db)
        sqlite3_close_v2(_db);
    _db = nullptr;
    _totalBytes = 0;
    _entriesCount = 0;
    _environments.clear();
}

bool OsmAnd::MapTileDiskCache_P::recreateLocked()
{
    closeLocked();
    QFile::remove(owner->filePath);
    QFile::remove(owner->filePath + QStringLiteral("-journal"));
    return openLocked(false);
}

void OsmAnd::MapTileDiskCache_P::recountLocked()
{
    _totalBytes = 0;
    _entriesCount = 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(_db, "SELECT count(*), total(size) FROM tiles", -1, &stmt, nullptr) == SQLITE_OK
        && sqlite3_step(stmt) == SQLITE_ROW)
    {
        _entriesCount = sqlite3_column_int(stmt, 0);
        _totalBytes = static_cast<int64_t>(sqlite3_column_double(stmt, 1));
    }
    sqlite3_finalize(stmt);
}

void OsmAnd::MapTileDiskCache_P::loadEnvironmentsLocked()
{
    _environments.clear();
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(_db, "SELECT hash, used FROM environments", -1, &stmt, nullptr) != SQLITE_OK)
        return;
    while (sqlite3_step(stmt) == SQLITE_ROW)
        _environments.insert(static_cast<uint64_t>(sqlite3_column_int64(stmt, 0)), sqlite3_column_int64(stmt, 1));
    sqlite3_finalize(stmt);
}

bool OsmAnd::MapTileDiskCache_P::isOpen() const
{
    QMutexLocker scopedLocker(&_mutex);
    return _db != nullptr;
}

void OsmAnd::MapTileDiskCache_P::setLimit(const int64_t limitBytes)
{
    QMutexLocker scopedLocker(&_mutex);
    _limit = limitBytes;
    if (_db && _totalBytes > _limit)
    {
        evictLocked(_limit);
        vacuumLocked();
    }
}

int64_t OsmAnd::MapTileDiskCache_P::getLimit() const
{
    QMutexLocker scopedLocker(&_mutex);
    return _limit;
}

void OsmAnd::MapTileDiskCache_P::setKeptEnvironments(const int count)
{
    QMutexLocker scopedLocker(&_mutex);
    _keptEnvironments = qMax(1, count);
    if (_db && dropEnvironmentsLocked() > 0)
        vacuumLocked();
}

int64_t OsmAnd::MapTileDiskCache_P::getTotalBytes() const
{
    QMutexLocker scopedLocker(&_mutex);
    return _totalBytes;
}

int OsmAnd::MapTileDiskCache_P::getEntriesCount() const
{
    QMutexLocker scopedLocker(&_mutex);
    return _entriesCount;
}

void OsmAnd::MapTileDiskCache_P::clear()
{
    QMutexLocker scopedLocker(&_mutex);
    if (!_db)
        return;
    exec("DELETE FROM tiles");
    recountLocked();
    vacuumLocked();
}

void OsmAnd::MapTileDiskCache_P::vacuumLocked()
{
    // give the freed pages back to the file system
    exec("PRAGMA incremental_vacuum");
}

void OsmAnd::MapTileDiskCache_P::useEnvironment(const uint64_t environment)
{
    QMutexLocker scopedLocker(&_mutex);
    if (!_db)
        return;
    const auto t = now();
    const auto it = _environments.constFind(environment);
    const bool isNew = it == _environments.cend();
    // called for every tile: write only when the environment is new or its use time is stale
    if (!isNew && t - *it < MapTileDiskCache::AccessTimeResolutionSeconds)
        return;

    sqlite3_stmt* stmt = nullptr;
    int rc = SQLITE_OK;
    if (sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO environments (hash, used) VALUES (?1, ?2)", -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, asSigned(environment));
        sqlite3_bind_int64(stmt, 2, t);
        rc = sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);
    checkError(rc);
    if (!_db)
        return;
    _environments.insert(environment, t);
    if (isNew && dropEnvironmentsLocked() > 0)
        vacuumLocked();
}

int OsmAnd::MapTileDiskCache_P::dropEnvironmentsLocked()
{
    if (_environments.size() <= _keptEnvironments)
        return 0;
    QList< QPair<int64_t, uint64_t> > byUse;
    for (auto it = _environments.cbegin(); it != _environments.cend(); ++it)
        byUse.push_back(qMakePair(it.value(), it.key()));
    std::sort(byUse.begin(), byUse.end(),
        []
        (const QPair<int64_t, uint64_t>& l, const QPair<int64_t, uint64_t>& r) -> bool
        {
            return l.first > r.first;
        });

    const auto t = now();
    int deleted = 0;
    for (int i = _keptEnvironments; i < byUse.size(); i++)
    {
        if (t - byUse[i].first < EnvironmentGraceSeconds)
            continue;
        const auto hash = byUse[i].second;
        sqlite3_stmt* stmt = nullptr;
        for (const char* sql : { "DELETE FROM tiles WHERE environment = ?1", "DELETE FROM environments WHERE hash = ?1" })
        {
            if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
            {
                sqlite3_bind_int64(stmt, 1, asSigned(hash));
                if (sqlite3_step(stmt) == SQLITE_DONE)
                    deleted += sqlite3_changes(_db);
            }
            sqlite3_finalize(stmt);
            stmt = nullptr;
        }
        _environments.remove(hash);
    }
    if (deleted > 0)
        recountLocked();
    return deleted;
}

int OsmAnd::MapTileDiskCache_P::deleteAreaLocked(const AreaI& bbox31)
{
    sqlite3_stmt* stmt = nullptr;
    const char* const sql =
        "DELETE FROM tiles WHERE"
        " (x << (31 - zoom)) <= ?4 AND ((x + 1) << (31 - zoom)) > ?2"
        " AND (y << (31 - zoom)) <= ?3 AND ((y + 1) << (31 - zoom)) > ?1";
    int deleted = 0;
    if (sqlite3_prepare_v2(_db, sql, -1, &stmt, nullptr) == SQLITE_OK)
    {
        sqlite3_bind_int64(stmt, 1, bbox31.top());
        sqlite3_bind_int64(stmt, 2, bbox31.left());
        sqlite3_bind_int64(stmt, 3, bbox31.bottom());
        sqlite3_bind_int64(stmt, 4, bbox31.right());
        const auto rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE)
            deleted = sqlite3_changes(_db);
        sqlite3_finalize(stmt);
        checkError(rc);
        return deleted;
    }
    sqlite3_finalize(stmt);
    return deleted;
}

int OsmAnd::MapTileDiskCache_P::syncMaps(const QList<MapFile>& maps)
{
    QMutexLocker scopedLocker(&_mutex);
    if (!_db)
        return 0;

    QHash<QString, MapFile> stored;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(_db, "SELECT path, size, timestamp, top, left, bottom, right FROM maps", -1, &stmt, nullptr) == SQLITE_OK)
    {
        while (sqlite3_step(stmt) == SQLITE_ROW)
        {
            MapFile f;
            f.path = QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            f.size = sqlite3_column_int64(stmt, 1);
            f.timestamp = sqlite3_column_int64(stmt, 2);
            f.bbox31 = AreaI(sqlite3_column_int(stmt, 3), sqlite3_column_int(stmt, 4),
                sqlite3_column_int(stmt, 5), sqlite3_column_int(stmt, 6));
            stored.insert(f.path, f);
        }
    }
    sqlite3_finalize(stmt);

    QList<AreaI> changedAreas;
    QSet<QString> current;
    bool listChanged = stored.size() != maps.size();
    for (const auto& f : maps)
    {
        current.insert(f.path);
        const auto it = stored.constFind(f.path);
        if (it == stored.cend())
        {
            changedAreas.push_back(f.bbox31);
            listChanged = true;
        }
        else if (it->size != f.size || it->timestamp != f.timestamp || it->bbox31 != f.bbox31)
        {
            changedAreas.push_back(it->bbox31);
            changedAreas.push_back(f.bbox31);
            listChanged = true;
        }
    }
    for (const auto& f : stored)
        if (!current.contains(f.path))
            changedAreas.push_back(f.bbox31);
    if (!listChanged && changedAreas.isEmpty())
        return 0;

    int deleted = 0;
    exec("BEGIN");
    if (stored.isEmpty())
    {
        // no record of the maps the entries were made from
        if (sqlite3_exec(_db, "DELETE FROM tiles", nullptr, nullptr, nullptr) == SQLITE_OK)
            deleted += sqlite3_changes(_db);
    }
    else
    {
        for (const auto& area : changedAreas)
            deleted += deleteAreaLocked(area);
    }
    exec("DELETE FROM maps");
    if (_db && sqlite3_prepare_v2(_db, "INSERT OR REPLACE INTO maps (path, size, timestamp, top, left, bottom, right) "
        "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)", -1, &stmt, nullptr) == SQLITE_OK)
    {
        for (const auto& f : maps)
        {
            const auto path = f.path.toUtf8();
            sqlite3_bind_text(stmt, 1, path.constData(), path.size(), SQLITE_TRANSIENT);
            sqlite3_bind_int64(stmt, 2, f.size);
            sqlite3_bind_int64(stmt, 3, f.timestamp);
            sqlite3_bind_int(stmt, 4, f.bbox31.top());
            sqlite3_bind_int(stmt, 5, f.bbox31.left());
            sqlite3_bind_int(stmt, 6, f.bbox31.bottom());
            sqlite3_bind_int(stmt, 7, f.bbox31.right());
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
    }
    sqlite3_finalize(stmt);
    exec("COMMIT");
    if (!_db)
        return deleted;
    if (deleted > 0)
    {
        recountLocked();
        vacuumLocked();
    }
    return deleted;
}

bool OsmAnd::MapTileDiskCache_P::bindKey(sqlite3_stmt* stmt, const Key& key) const
{
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    return sqlite3_bind_int64(stmt, 1, asSigned(key.environment)) == SQLITE_OK
        && sqlite3_bind_int(stmt, 2, static_cast<int>(key.kind)) == SQLITE_OK
        && sqlite3_bind_int(stmt, 3, static_cast<int>(key.zoom)) == SQLITE_OK
        && sqlite3_bind_int(stmt, 4, key.tileId.x) == SQLITE_OK
        && sqlite3_bind_int(stmt, 5, key.tileId.y) == SQLITE_OK
        && sqlite3_bind_int(stmt, 6, key.variant) == SQLITE_OK;
}

bool OsmAnd::MapTileDiskCache_P::contains(const Key& key) const
{
    QMutexLocker scopedLocker(&_mutex);
    if (!_db || !MapTileDiskCache::isZoomCached(key.zoom) || !bindKey(_containsStmt, key))
        return false;
    const bool found = sqlite3_step(_containsStmt) == SQLITE_ROW;
    sqlite3_reset(_containsStmt);
    return found;
}

QByteArray OsmAnd::MapTileDiskCache_P::get(const Key& key)
{
    QMutexLocker scopedLocker(&_mutex);
    QByteArray data;
    if (!_db || !MapTileDiskCache::isZoomCached(key.zoom) || !bindKey(_getStmt, key))
        return data;
    const auto rc = sqlite3_step(_getStmt);
    if (rc != SQLITE_ROW)
    {
        sqlite3_reset(_getStmt);
        if (rc != SQLITE_DONE)
            checkError(rc);
        return data;
    }
    const auto rowId = sqlite3_column_int64(_getStmt, 0);
    const auto atime = sqlite3_column_int64(_getStmt, 1);
    const auto blob = sqlite3_column_blob(_getStmt, 2);
    const auto size = sqlite3_column_bytes(_getStmt, 2);
    if (blob && size > 0)
        data = QByteArray(static_cast<const char*>(blob), size);
    sqlite3_reset(_getStmt);

    const auto t = now();
    if (t - atime >= MapTileDiskCache::AccessTimeResolutionSeconds)
    {
        sqlite3_reset(_touchStmt);
        sqlite3_bind_int64(_touchStmt, 1, t);
        sqlite3_bind_int64(_touchStmt, 2, rowId);
        sqlite3_step(_touchStmt);
        sqlite3_reset(_touchStmt);
    }
    return data;
}

bool OsmAnd::MapTileDiskCache_P::put(const Key& key, const QByteArray& data)
{
    QMutexLocker scopedLocker(&_mutex);
    // one entry may not take more than 1/16 of the cache
    if (!_db || data.isEmpty() || !MapTileDiskCache::isZoomCached(key.zoom) || data.size() > _limit / 16)
        return false;
    // an environment that was dropped (or never announced) is not stored: its renderer is going away
    if (!_environments.contains(key.environment))
        return false;

    int64_t replacedSize = -1;
    if (bindKey(_sizeStmt, key) && sqlite3_step(_sizeStmt) == SQLITE_ROW)
        replacedSize = sqlite3_column_int64(_sizeStmt, 0);
    sqlite3_reset(_sizeStmt);

    if (!bindKey(_insertStmt, key))
        return false;
    sqlite3_bind_int64(_insertStmt, 7, now());
    sqlite3_bind_int(_insertStmt, 8, MapTileDiskCache::getZoomWeight(key.zoom));
    sqlite3_bind_int64(_insertStmt, 9, data.size());
    sqlite3_bind_blob(_insertStmt, 10, data.constData(), data.size(), SQLITE_STATIC);
    const auto rc = sqlite3_step(_insertStmt);
    sqlite3_reset(_insertStmt);
    sqlite3_clear_bindings(_insertStmt);
    if (rc != SQLITE_DONE)
    {
        checkError(rc);
        return false;
    }
    if (replacedSize >= 0)
    {
        _totalBytes -= replacedSize;
        _entriesCount--;
    }
    _totalBytes += data.size();
    _entriesCount++;
    if (_totalBytes > _limit)
        evictLocked(_limit - _limit / 10);
    return true;
}

int OsmAnd::MapTileDiskCache_P::evictLocked(const int64_t targetBytes)
{
    // idle time divided by the zoom weight: the largest goes first
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(_db, "SELECT rowid, size FROM tiles ORDER BY (?1 - atime + 1.0) / weight DESC, zoom DESC",
        -1, &stmt, nullptr) != SQLITE_OK)
    {
        return 0;
    }
    sqlite3_bind_int64(stmt, 1, now());
    QList<int64_t> rowIds;
    int64_t remaining = _totalBytes;
    while (remaining > targetBytes && sqlite3_step(stmt) == SQLITE_ROW)
    {
        rowIds.push_back(sqlite3_column_int64(stmt, 0));
        remaining -= sqlite3_column_int64(stmt, 1);
    }
    sqlite3_finalize(stmt);
    if (rowIds.isEmpty())
        return 0;

    exec("BEGIN");
    if (_db && sqlite3_prepare_v2(_db, "DELETE FROM tiles WHERE rowid = ?1", -1, &stmt, nullptr) == SQLITE_OK)
    {
        for (const auto rowId : rowIds)
        {
            sqlite3_bind_int64(stmt, 1, rowId);
            sqlite3_step(stmt);
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }
    exec("COMMIT");
    if (_db)
        recountLocked();
    return rowIds.size();
}

void OsmAnd::MapTileDiskCache_P::setClock(const std::function<int64_t()>& clock)
{
    QMutexLocker scopedLocker(&_mutex);
    _clock = clock;
}
