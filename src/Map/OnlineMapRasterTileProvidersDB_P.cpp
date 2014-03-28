#include "OnlineMapRasterTileProvidersDB_P.h"
#include "OnlineMapRasterTileProvidersDB.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

#include "OnlineMapRasterTileProvider.h"
#include "QCachingIterator.h"
#include "QKeyValueIterator.h"

OsmAnd::OnlineMapRasterTileProvidersDB_P::OnlineMapRasterTileProvidersDB_P(OnlineMapRasterTileProvidersDB* owner_)
    : owner(owner_)
{
}

OsmAnd::OnlineMapRasterTileProvidersDB_P::~OnlineMapRasterTileProvidersDB_P()
{
}

bool OsmAnd::OnlineMapRasterTileProvidersDB_P::add(const OnlineMapRasterTileProvidersDB::Entry& entry)
{
    if(_entries.contains(entry.id))
        return false;

    _entries.insert(entry.id, entry);
    return true;
}

bool OsmAnd::OnlineMapRasterTileProvidersDB_P::remove(const QString& id)
{
    return (_entries.remove(id) > 0);
}

bool OsmAnd::OnlineMapRasterTileProvidersDB_P::saveTo(const QString& filePath) const
{
    const auto dbFilepath = QFileInfo(filePath).absoluteFilePath();
    const auto connectionName = QLatin1String("online-tile-providers-db:") + dbFilepath;
    bool ok;

    {
        QSqlDatabase sqliteDb =
            (!QSqlDatabase::contains(connectionName))
            ? QSqlDatabase::addDatabase("QSQLITE", connectionName)
            : QSqlDatabase::database(connectionName);
        sqliteDb.setDatabaseName(dbFilepath);
        ok = sqliteDb.open();
        while(ok)
        {
            QSqlQuery q(sqliteDb);

            ok = q.exec("DROP TABLE IF EXISTS providers");
            if(!ok)
                break;

            ok = q.exec(
                "CREATE TABLE providers ("
                "    id TEXT,"
                "    name TEXT,"
                "    urlPattern TEXT,"
                "    minZoom INTEGER,"
                "    maxZoom INTEGER,"
                "    maxConcurrentDownloads INTEGER,"
                "    tileSize INTEGER,"
                "    alphaChannelData INTEGER"
                ")");
            if(!ok)
                break;

            ok = sqliteDb.commit();
            if(!ok)
                break;

            QSqlQuery insertEntryQuery(sqliteDb);
            ok = insertEntryQuery.prepare("INSERT INTO providers (id, name, urlPattern, minZoom, maxZoom, maxConcurrentDownloads, tileSize, alphaChannelData) VALUES ( ?, ?, ?, ?, ?, ?, ?, ? )");
            if(!ok)
                break;

            for(const auto& entry : constOf(_entries))
            {
                insertEntryQuery.addBindValue(entry.id);
                insertEntryQuery.addBindValue(entry.name);
                insertEntryQuery.addBindValue(entry.urlPattern);
                insertEntryQuery.addBindValue(entry.minZoom);
                insertEntryQuery.addBindValue(entry.maxZoom);
                insertEntryQuery.addBindValue(entry.maxConcurrentDownloads);
                insertEntryQuery.addBindValue(entry.tileSize);
                switch(entry.alphaChannelData)
                {
                case AlphaChannelData::Present:
                    insertEntryQuery.addBindValue(1);
                    break;
                case AlphaChannelData::NotPresent:
                    insertEntryQuery.addBindValue(0);
                    break;
                case AlphaChannelData::Undefined:
                    insertEntryQuery.addBindValue(-1);
                    break;
                }

                ok = insertEntryQuery.exec();
                if(!ok)
                    break;
            }
            if(ok)
                ok = sqliteDb.commit();;

            sqliteDb.close();
            break;
        }
    }

    QSqlDatabase::removeDatabase(connectionName);

    return ok;
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> OsmAnd::OnlineMapRasterTileProvidersDB_P::createProvider(const QString& providerId) const
{
    std::shared_ptr<OsmAnd::OnlineMapRasterTileProvider> result;

    const auto& citEntry = _entries.constFind(providerId);
    if(citEntry == _entries.cend())
        return result;

    const auto& entry = *citEntry;
    result.reset(new OnlineMapRasterTileProvider(
        entry.id,
        entry.urlPattern,
        entry.minZoom,
        entry.maxZoom,
        entry.maxConcurrentDownloads,
        entry.tileSize,
        entry.alphaChannelData));

    return result;
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB_P::createDefaultDB()
{
    std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> db(new OnlineMapRasterTileProvidersDB());

    // Mapnik (hosted at OsmAnd)
    OnlineMapRasterTileProvidersDB::Entry entryMapnikAtOsmAnd = {
        QLatin1String("mapnik_osmand"),
        QLatin1String("Mapnik (OsmAnd)"),
        QLatin1String("http://mapnik.osmand.net/${zoom}/${x}/${y}.png"),
        ZoomLevel0, ZoomLevel19,
        0,
        256,
        AlphaChannelData::NotPresent
    };
    db->add(entryMapnikAtOsmAnd);

    return db;
}

std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> OsmAnd::OnlineMapRasterTileProvidersDB_P::loadFrom(const QString& filePath)
{
    std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB> db(new OnlineMapRasterTileProvidersDB());

    const auto dbFilepath = QFileInfo(filePath).absoluteFilePath();
    const auto connectionName = QLatin1String("online-tile-providers-db:") + dbFilepath;
    bool ok;

    {
        QSqlDatabase sqliteDb =
            (!QSqlDatabase::contains(connectionName))
            ? QSqlDatabase::addDatabase("QSQLITE", connectionName)
            : QSqlDatabase::database(connectionName);
        sqliteDb.setDatabaseName(dbFilepath);
        ok = sqliteDb.open();
        while(ok)
        {
            QSqlQuery q(sqliteDb);

            ok = q.exec("SELECT * FROM providers");
            while(ok && q.next())
            {
                OnlineMapRasterTileProvidersDB::Entry entry;

                entry.id = q.value(QLatin1String("id")).toString();
                entry.name = q.value(QLatin1String("name")).toString();
                entry.urlPattern = q.value(QLatin1String("urlPattern")).toString();
                entry.minZoom = static_cast<ZoomLevel>(q.value(QLatin1String("minZoom")).toInt());
                entry.maxZoom = static_cast<ZoomLevel>(q.value(QLatin1String("maxZoom")).toInt());
                entry.maxConcurrentDownloads = q.value(QLatin1String("maxConcurrentDownloads")).toInt();
                entry.tileSize = q.value(QLatin1String("tileSize")).toInt();
                const auto alphaChannelData = q.value(QLatin1String("alphaChannelData")).toInt();
                switch(alphaChannelData)
                {
                case -1:
                    entry.alphaChannelData = AlphaChannelData::Undefined;
                    break;
                case 1:
                    entry.alphaChannelData = AlphaChannelData::Present;
                    break;
                case 0:
                    entry.alphaChannelData = AlphaChannelData::NotPresent;
                    break;
                }

                ok = db->add(entry);
            }

            if(!ok)
                break;

            sqliteDb.close();
            break;
        }
    }

    QSqlDatabase::removeDatabase(connectionName);

    if(!ok)
        return std::shared_ptr<OsmAnd::OnlineMapRasterTileProvidersDB>();
    return db;
}
