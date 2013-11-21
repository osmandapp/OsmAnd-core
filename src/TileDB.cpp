#include "TileDB.h"

#include <cassert>

#include <OsmAndCore/QtExtensions.h>
#include <QtSql>

#include "Logging.h"
#include "Utilities.h"

OsmAnd::TileDB::TileDB( const QDir& dataPath_, const QString& indexFilename_/* = QString()*/ )
    : _indexMutex(QMutex::Recursive)
    , dataPath(dataPath_)
    , indexFilename(indexFilename_)
{
    auto indexConnectionName = QLatin1String("tiledb-sqlite-index:") + dataPath_.absolutePath();

    if(!QSqlDatabase::contains(indexConnectionName))
        _indexDb = QSqlDatabase::addDatabase("QSQLITE", indexConnectionName);
    else
        _indexDb = QSqlDatabase::database(indexConnectionName);
}

OsmAnd::TileDB::~TileDB()
{
    if(_indexDb.isOpen())
        _indexDb.close();
}

bool OsmAnd::TileDB::openIndex()
{
    QMutexLocker scopeLock(&_indexMutex);

    bool ok;
    bool shouldRebuild = indexFilename.isEmpty() || !QFile(indexFilename).exists();

    _indexDb.setDatabaseName(indexFilename.isEmpty() ? ":memory:" : indexFilename);
    ok = _indexDb.open();
    if(!ok)
    {
        LogPrintf(LogSeverityLevel::Error, "Failed to open TileDB index from '%s': %s", qPrintable(indexFilename), qPrintable(_indexDb.lastError().text()));
        return false;
    }

    if(shouldRebuild)
        rebuildIndex();

    return true;
}

bool OsmAnd::TileDB::rebuildIndex()
{
    QMutexLocker scopeLock(&_indexMutex);

    bool ok;

    // Open index database if it's not yet
    if(!_indexDb.isOpen())
    {
        if(!openIndex())
            return false;
    }
    QSqlQuery q(_indexDb);

    LogPrintf(LogSeverityLevel::Info, "Rebuilding index of '%s' tiledb...", qPrintable(dataPath.absolutePath()));
    auto beginTimestamp = std::chrono::steady_clock::now();

    // Recreate index db structure
    if(!indexFilename.isEmpty())
    {
        ok = q.exec("DROP TABLE IF EXISTS tiledb_files");
        assert(ok);
        ok = q.exec("DROP TABLE IF EXISTS tiledb_index");
        assert(ok);
        ok = q.exec("DROP INDEX IF EXISTS _tiledb_index");
        assert(ok);
    }
    ok = q.exec(
        "CREATE TABLE tiledb_files ("
        "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "    filename TEXT"
        ")");
    assert(ok);
    ok = q.exec(
        "CREATE TABLE tiledb_index ("
        "    xMin INTEGER,"
        "    yMin INTEGER,"
        "    xMax INTEGER,"
        "    yMax INTEGER,"
        "    zoom INTEGER,"
        "    id INTEGER"
        ")");
    assert(ok);
    ok = q.exec(
        "CREATE INDEX _tiledb_index"
        "    ON tiledb_index(xMin, yMin, xMax, yMax, zoom)");
    assert(ok);
    _indexDb.commit();

    QSqlQuery registerFileQuery(_indexDb);
    ok = registerFileQuery.prepare("INSERT INTO tiledb_files (filename) VALUES ( ? )");
    assert(ok);

    QSqlQuery insertTileQuery(_indexDb);
    ok = insertTileQuery.prepare("INSERT INTO tiledb_index (xMin, yMin, xMax, yMax, zoom, id) VALUES ( ?, ?, ?, ?, ?, ? )");
    assert(ok);

    // Index TileDBs
    QFileInfoList files;
    Utilities::findFiles(dataPath, QStringList() << "*", files);
    for(auto itFile = files.cbegin(); itFile != files.cend(); ++itFile)
    {
        const auto& file = *itFile;
        const auto dbFilename = file.absoluteFilePath();

        const auto connectionName = QLatin1String("tiledb-sqlite:") + dbFilename;
        QSqlDatabase db;
        if(!QSqlDatabase::contains(connectionName))
            db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        else
            db = QSqlDatabase::database(connectionName);
        db.setDatabaseName(dbFilename);
        if(!db.open())
        {
            LogPrintf(LogSeverityLevel::Error, "Failed to open TileDB from '%s': %s", qPrintable(dbFilename), qPrintable(db.lastError().text()));
            continue;
        }

        // Register new file
        registerFileQuery.addBindValue(dbFilename);
        ok = registerFileQuery.exec();
        assert(ok);
        auto fileId = registerFileQuery.lastInsertId();

        // For each zoom, query min-max of tile coordinates
        QSqlQuery minMaxQuery("SELECT zoom, xMin, yMin, xMax, yMax FROM bounds", db);
        ok = minMaxQuery.exec();
        assert(ok);
        while(minMaxQuery.next())
        {
            int32_t zoom = minMaxQuery.value(0).toInt();
            int32_t xMin = minMaxQuery.value(1).toInt();
            int32_t yMin = minMaxQuery.value(2).toInt();
            int32_t xMax = minMaxQuery.value(3).toInt();
            int32_t yMax = minMaxQuery.value(4).toInt();

            insertTileQuery.addBindValue(xMin);
            insertTileQuery.addBindValue(yMin);
            insertTileQuery.addBindValue(xMax);
            insertTileQuery.addBindValue(yMax);
            insertTileQuery.addBindValue(zoom);
            insertTileQuery.addBindValue(fileId);

            ok = insertTileQuery.exec();
            assert(ok);
        }

        db.close();
    }

    _indexDb.commit();

    auto endTimestamp = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast< std::chrono::duration<uint64_t, std::milli> >(endTimestamp - beginTimestamp).count();
    LogPrintf(LogSeverityLevel::Info, "Finished indexing '%s', took %lldms, average %lldms/db", qPrintable(dataPath.absolutePath()), duration, duration / files.length());

    return true;
}

bool OsmAnd::TileDB::obtainTileData( const TileId tileId, const ZoomLevel zoom, QByteArray& data )
{
    QMutexLocker scopeLock(&_indexMutex);

    // Check that index is available
    if(!_indexDb.isOpen())
    {
        if(!openIndex())
            return false;
    }

    QSqlQuery query(_indexDb);
    query.prepare("SELECT filename FROM tiledb_files WHERE id IN (SELECT id FROM tiledb_index WHERE xMin<=? AND xMax>=? AND yMin<=? AND yMax>=? AND zoom=?)");
    query.addBindValue(tileId.x);
    query.addBindValue(tileId.x);
    query.addBindValue(tileId.y);
    query.addBindValue(tileId.y);
    query.addBindValue(zoom);
    if(!query.exec())
        return false;

    bool hit = false;
    while(!hit && query.next())
    {
        const auto dbFilename = query.value(0).toString();

        //TODO: manage access to database
        //QMutexLocker scopeLock(&dbEntry->mutex);

        // Open database
        const auto connectionName = QLatin1String("tiledb-sqlite:") + dbFilename;
        QSqlDatabase db;
        if(!QSqlDatabase::contains(connectionName))
            db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        else
            db = QSqlDatabase::database(connectionName);
        db.setDatabaseName(dbFilename);
        if(!db.open())
        {
            LogPrintf(LogSeverityLevel::Error, "Failed to open TileDB index from '%s': %s", qPrintable(dbFilename), qPrintable(db.lastError().text()));
            return false;
        }

        // Get tile from
        QSqlQuery query(db);
        query.prepare("SELECT data FROM tiles WHERE x=? AND y=? AND zoom=?");
        query.addBindValue(tileId.x);
        query.addBindValue(tileId.y);
        query.addBindValue(static_cast<int>(zoom));
        if(query.exec() && query.next())
        {
            data = query.value(0).toByteArray();
            hit = true;
        }

        // Close database
        db.close();
    }
    
    return hit;
}
