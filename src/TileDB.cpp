#include "TileDB.h"

#include <assert.h>

#include <QtSql>

#include "Logging.h"
#include "Utilities.h"

OsmAnd::TileDB::TileDB( const QDir& dataPath_, const QString& indexFilename_/* = QString()*/ )
    : _indexMutex(QMutex::Recursive)
    , dataPath(dataPath_)
    , indexFilename(indexFilename_)
{
    auto indexConnectionName = QString::fromLatin1("tiledb-sqlite-index:") + dataPath_.absolutePath();

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
        "    x INTEGER,"
        "    y INTEGER,"
        "    zoom INTEGER,"
        "    id INTEGER"
        ")");
    assert(ok);
    ok = q.exec(
        "CREATE INDEX _tiledb_index"
        "    ON tiledb_index(x, y, zoom)");
    assert(ok);
    _indexDb.commit();

    QSqlQuery registerFileQuery(_indexDb);
    ok = registerFileQuery.prepare("INSERT INTO tiledb_files (filename) VALUES ( ? )");
    assert(ok);

    QSqlQuery insertTileQuery(_indexDb);
    ok = insertTileQuery.prepare("INSERT INTO tiledb_index (x, y, zoom, id) VALUES ( ?, ?, ?, ? )");
    assert(ok);

    // Index TileDBs
    QList< std::shared_ptr<QFileInfo> > files;
    Utilities::findFiles(dataPath, QStringList() << "*", files);
    for(auto itFile = files.begin(); itFile != files.end(); ++itFile)
    {
        const auto& file = *itFile;
        const auto dbFilename = file->absoluteFilePath();

        const auto connectionName = QString::fromLatin1("tiledb-sqlite:") + file->absoluteFilePath();
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

        // Query all tiles and add them
        QSqlQuery query("SELECT x, y, zoom FROM tiles", db);
        while(query.next())
        {
            int32_t x = query.value(0).toInt();
            int32_t y = query.value(1).toInt();
            int32_t zoom = query.value(2).toInt();

            insertTileQuery.addBindValue(x);
            insertTileQuery.addBindValue(y);
            insertTileQuery.addBindValue(zoom);
            insertTileQuery.addBindValue(fileId);

            ok = insertTileQuery.exec();
            auto e = insertTileQuery.lastError().text();
            assert(ok);
        }

        db.close();
    }

    _indexDb.commit();

    return true;
}

bool OsmAnd::TileDB::obtainTileData( const TileId& tileId, const uint32_t& zoom, QByteArray& data )
{
    QString dbFilename;

    {
        QMutexLocker scopeLock(&_indexMutex);

        // Check that index is available
        if(!_indexDb.isOpen())
        {
            if(!openIndex())
                return false;
        }

        QSqlQuery query(_indexDb);
        query.prepare("SELECT filename FROM tiledb_files WHERE id IN (SELECT id FROM tiledb_index WHERE x=? AND y=? AND zoom=?)");
        query.addBindValue(tileId.x);
        query.addBindValue(tileId.y);
        query.addBindValue(zoom);
        if(!query.exec() || !query.next())
            return false;
        dbFilename = query.value(0).toString();
    }

    {
        //TODO: manage access to database
        //QMutexLocker scopeLock(&dbEntry->mutex);

        // Open database
        const auto connectionName = QString::fromLatin1("tiledb-sqlite:") + dbFilename;
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
        query.addBindValue(zoom);
        bool hit = false;
        if(query.exec() && query.next())
        {
            data = query.value(0).toByteArray();
            hit = true;
        }

        // Close database
        db.close();

        return hit;
    }
}
