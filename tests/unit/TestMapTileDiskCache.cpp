#include <OsmAndCore/Map/MapTileDiskCache.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>

#include <sqlite3.h>

#include <cmath>
#include <memory>

using namespace OsmAnd;

namespace
{
    const uint64_t kDay = 0xDA1;
    const uint64_t kNight = 0x4167;
    const uint64_t kCar = 0xCA2;

    MapTileDiskCache::Key key(const int x, const int y, const ZoomLevel zoom,
        const uint64_t environment = kDay,
        const MapTileDiskCache::DataKind kind = MapTileDiskCache::DataKind::RasterImage,
        const int variant = 0)
    {
        MapTileDiskCache::Key k;
        k.environment = environment;
        k.kind = kind;
        k.tileId = TileId::fromXY(x, y);
        k.zoom = zoom;
        k.variant = variant;
        return k;
    }

    QByteArray blob(const int size, const char fill = 'x')
    {
        return QByteArray(size, fill);
    }

    // tile (x, y) at zoom z in 31-bit coordinates
    AreaI tileArea(const int x, const int y, const int zoom)
    {
        const int shift = 31 - zoom;
        return AreaI(y << shift, x << shift, ((y + 1) << shift) - 1, ((x + 1) << shift) - 1);
    }

    MapTileDiskCache::MapFile map(const QString& path, const AreaI& bbox31, const int64_t size = 1000, const int64_t timestamp = 1)
    {
        MapTileDiskCache::MapFile f;
        f.path = path;
        f.size = size;
        f.timestamp = timestamp;
        f.bbox31 = bbox31;
        return f;
    }
}

class TestMapTileDiskCache : public QObject
{
    Q_OBJECT

    QTemporaryDir _dir;
    int64_t _now;

    QString path() const
    {
        return _dir.filePath(QStringLiteral("tiles.db"));
    }

    std::unique_ptr<MapTileDiskCache> open(const int64_t limit = 1024 * 1024)
    {
        std::unique_ptr<MapTileDiskCache> cache(new MapTileDiskCache(path(), limit));
        cache->setClock([this]() { return _now; });
        return cache;
    }

private slots:
    void init()
    {
        _now = 1000000;
        QFile::remove(path());
    }

    void putGetAndReopen()
    {
        {
            auto cache = open();
            QVERIFY(cache->isOpen());
            cache->useEnvironment(kDay);
            QVERIFY(cache->put(key(10, 20, ZoomLevel13), blob(100, 'a'), 0));
            QVERIFY(cache->put(key(10, 20, ZoomLevel13, kDay, MapTileDiskCache::DataKind::Symbols), blob(50, 's'), 0));
            QCOMPARE(cache->getEntriesCount(), 2);
            QCOMPARE(cache->getTotalBytes(), int64_t(150));
            QVERIFY(cache->contains(key(10, 20, ZoomLevel13)));
            QCOMPARE(cache->get(key(10, 20, ZoomLevel13)), blob(100, 'a'));
        }
        auto cache = open();
        QCOMPARE(cache->getEntriesCount(), 2);
        QCOMPARE(cache->getTotalBytes(), int64_t(150));
        QCOMPARE(cache->get(key(10, 20, ZoomLevel13, kDay, MapTileDiskCache::DataKind::Symbols)), blob(50, 's'));
    }

    void keyFieldsAllMatter()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(1, 2, ZoomLevel10, kDay, MapTileDiskCache::DataKind::RasterImage, 10), blob(10), 0));
        QVERIFY(cache->contains(key(1, 2, ZoomLevel10, kDay, MapTileDiskCache::DataKind::RasterImage, 10)));
        QVERIFY(!cache->contains(key(1, 2, ZoomLevel10, kDay, MapTileDiskCache::DataKind::RasterImage, 11)));   // detailed zoom
        QVERIFY(!cache->contains(key(1, 2, ZoomLevel10, kDay, MapTileDiskCache::DataKind::Symbols, 10)));
        QVERIFY(!cache->contains(key(1, 2, ZoomLevel10, kNight, MapTileDiskCache::DataKind::RasterImage, 10)));
        QVERIFY(!cache->contains(key(2, 1, ZoomLevel10, kDay, MapTileDiskCache::DataKind::RasterImage, 10)));
        QVERIFY(!cache->contains(key(1, 2, ZoomLevel11, kDay, MapTileDiskCache::DataKind::RasterImage, 10)));
    }

    void replaceKeepsAccounting()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(100), 0));
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(30, 'b'), 0));
        QCOMPARE(cache->getEntriesCount(), 1);
        QCOMPARE(cache->getTotalBytes(), int64_t(30));
        QCOMPARE(cache->get(key(1, 1, ZoomLevel12)), blob(30, 'b'));
    }

    void onlyCachedZoomsAndSaneSizes()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        QVERIFY(!cache->put(key(1, 1, ZoomLevel16), blob(10), 0));
        QVERIFY(!cache->put(key(1, 1, ZoomLevel15), QByteArray(), 0));
        QVERIFY(!cache->put(key(1, 1, ZoomLevel15), blob(1001), 0));   // more than 1/16 of the limit
        QVERIFY(cache->put(key(1, 1, ZoomLevel15), blob(1000), 0));
        QCOMPARE(cache->getEntriesCount(), 1);
    }

    void unknownEnvironmentIsNotStored()
    {
        auto cache = open();
        QVERIFY(!cache->put(key(1, 1, ZoomLevel12, kNight), blob(10), 0));
        cache->useEnvironment(kNight);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kNight), blob(10), 0));
    }

    // over 90% of the limit the lowest weights go, down to 60%; with equal zoom and cost the oldest are lowest
    void evictsLowestWeight()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        for (int i = 0; i < 14; i++)
        {
            _now += 3600;
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000), 0));
        }
        QCOMPARE(cache->getEntriesCount(), 14);   // 14000 <= 14400
        _now += 3600;
        QVERIFY(cache->put(key(14, 0, ZoomLevel15), blob(1000), 0));
        QCOMPARE(cache->getTotalBytes(), int64_t(9000));   // 15000 -> under 9600
        for (int i = 0; i < 6; i++)
            QVERIFY(!cache->contains(key(i, 0, ZoomLevel15)));
        for (int i = 6; i < 15; i++)
            QVERIFY(cache->contains(key(i, 0, ZoomLevel15)));
    }

    // a hit makes the entry as valuable as one added now
    void readKeepsEntryAlive()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        for (int i = 0; i < 14; i++)
        {
            _now += 3600;
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000), 0));
        }
        _now += 3600;
        QVERIFY(!cache->get(key(0, 0, ZoomLevel15)).isEmpty());
        _now += 3600;
        QVERIFY(cache->put(key(14, 0, ZoomLevel15), blob(1000), 0));
        QVERIFY(cache->contains(key(0, 0, ZoomLevel15)));
        for (int i = 1; i < 7; i++)
            QVERIFY(!cache->contains(key(i, 0, ZoomLevel15)));
    }

    void weightFormula()
    {
        const int64_t week = MapTileDiskCache::HalfLifeDays * 86400;
        const int64_t t = 100 * week;
        const auto w = MapTileDiskCache::getInitialWeight(t, ZoomLevel15, 0);
        // a week without use costs 1
        QCOMPARE(MapTileDiskCache::getInitialWeight(t + week, ZoomLevel15, 0) - w, 1.0);
        // zoom: z9-z14 x4, z12 x8
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel8), 1);
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel9), 4);
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel14), 4);
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel12), 8);
        QCOMPARE(MapTileDiskCache::getInitialWeight(t, ZoomLevel11, 0) - w, 2.0);
        QCOMPARE(MapTileDiskCache::getInitialWeight(t, ZoomLevel12, 0) - w, 3.0);
        // 255 ms to draw: 0.5 * log2(256)
        QCOMPARE(MapTileDiskCache::getInitialWeight(t, ZoomLevel15, 255) - w, 4.0);

        // a second visit one half-life later: count 0.5 + 1
        const auto visited = MapTileDiskCache::getAccessedWeight(w, ZoomLevel15, 0, t, t + week, t + week);
        QVERIFY(qAbs(visited - (MapTileDiskCache::getInitialWeight(t + week, ZoomLevel15, 0) + std::log2(1.5))) < 1e-9);
        // two visits at once: count 2
        const auto twice = MapTileDiskCache::getAccessedWeight(w, ZoomLevel15, 0, t - 3600, t, t);
        QVERIFY(qAbs(twice - (w + 1.0)) < 1e-9);
        // accesses within a visit only bring the entry to "used now", the zoom and cost parts stay
        const auto w12 = MapTileDiskCache::getInitialWeight(t, ZoomLevel12, 100);
        const auto sameVisit = MapTileDiskCache::getAccessedWeight(w12, ZoomLevel12, 100, t, t + 60, t + 600);
        QVERIFY(qAbs(sameVisit - MapTileDiskCache::getInitialWeight(t + 600, ZoomLevel12, 100)) < 1e-9);
        // and never lower it
        QCOMPARE(MapTileDiskCache::getAccessedWeight(visited, ZoomLevel15, 0, t + week, t + week + 60, t + week + 60), visited);
    }

    // z12 is worth 3 weeks of not being used, z11 2 weeks
    void zoomWeights()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(0, 0, ZoomLevel12), blob(1000), 0));
        QVERIFY(cache->put(key(0, 0, ZoomLevel11), blob(1000), 0));
        _now += 15 * 86400;
        for (int i = 0; i < 13; i++)
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000), 0));
        QVERIFY(cache->contains(key(0, 0, ZoomLevel12)));
        QVERIFY(!cache->contains(key(0, 0, ZoomLevel11)));
        QCOMPARE(cache->getEntriesCount(), 9);
    }

    // a tile that took 1 s to draw is worth 5 weeks
    void costlyTilesLiveLonger()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(0, 0, ZoomLevel15), blob(1000), 1023));
        QVERIFY(cache->put(key(1, 0, ZoomLevel15), blob(1000), 0));
        _now += 28 * 86400;
        for (int i = 2; i < 15; i++)
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000), 0));
        QVERIFY(cache->contains(key(0, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(1, 0, ZoomLevel15)));
    }

    // visits on four days outweigh a week of not being used; many hits in one minute do not
    void visitsNotHitsCount()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(0, 0, ZoomLevel15), blob(1000), 0));   // home, seen every day
        for (int day = 1; day <= 3; day++)
        {
            _now += 86400;
            QVERIFY(!cache->get(key(0, 0, ZoomLevel15)).isEmpty());
            cache->flush();
        }
        QVERIFY(cache->put(key(1, 0, ZoomLevel15), blob(1000), 0));   // seen once, on the last day
        QVERIFY(cache->put(key(2, 0, ZoomLevel15), blob(1000), 0));   // zoomed in and out over it
        for (int i = 0; i < 20; i++)
        {
            _now += 3;
            QVERIFY(!cache->get(key(2, 0, ZoomLevel15)).isEmpty());
        }
        cache->flush();
        MapTileDiskCache::EntryInfo info;
        QVERIFY(cache->getEntryInfo(key(0, 0, ZoomLevel15), info));
        QCOMPARE(info.hits, 3);
        QVERIFY(cache->getEntryInfo(key(2, 0, ZoomLevel15), info));
        QCOMPARE(info.hits, 20);

        _now += 7 * 86400 + 3600;
        for (int i = 3; i < 15; i++)
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000), 0));
        QVERIFY(cache->contains(key(0, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(1, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(2, 0, ZoomLevel15)));
    }

    // hits are counted in memory and written by flush
    void accessStatsAreWrittenOnFlush()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        const auto added = _now;
        QVERIFY(cache->put(key(5, 5, ZoomLevel13), blob(100), 42));
        _now += 5;
        QVERIFY(!cache->get(key(5, 5, ZoomLevel13)).isEmpty());
        QVERIFY(!cache->get(key(5, 5, ZoomLevel13)).isEmpty());
        cache.reset();   // closing writes them too
        cache = open();
        MapTileDiskCache::EntryInfo info;
        QVERIFY(cache->getEntryInfo(key(5, 5, ZoomLevel13), info));
        QCOMPARE(info.added, added);
        QCOMPARE(info.lastAccess, added + 5);
        QCOMPARE(info.hits, 2);
        QCOMPARE(info.generationTimeMs, 42);
        QCOMPARE(info.size, 100);
    }

    // the data is made and written on the cache's thread
    void putLater()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        cache->putLater(key(7, 7, ZoomLevel14), []() { return blob(64, 'w'); }, 10);
        QTRY_VERIFY(cache->contains(key(7, 7, ZoomLevel14)));
        QCOMPARE(cache->get(key(7, 7, ZoomLevel14)), blob(64, 'w'));
    }

    // a cache that is only read writes its hits on its own thread too
    void hitsAreWrittenInBackground()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(8, 8, ZoomLevel14), blob(64), 0));
        _now += MapTileDiskCache::FlushSeconds;
        QVERIFY(!cache->get(key(8, 8, ZoomLevel14)).isEmpty());
        // read the file with another connection: only written data is there
        sqlite3* db = nullptr;
        QVERIFY(sqlite3_open_v2(path().toUtf8().constData(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK);
        sqlite3_busy_timeout(db, 1000);
        const auto readHits =
            [db]
            () -> int
            {
                sqlite3_stmt* stmt = nullptr;
                int hits = -1;
                if (sqlite3_prepare_v2(db, "SELECT hits FROM tiles", -1, &stmt, nullptr) == SQLITE_OK
                    && sqlite3_step(stmt) == SQLITE_ROW)
                {
                    hits = sqlite3_column_int(stmt, 0);
                }
                sqlite3_finalize(stmt);
                return hits;
            };
        QTRY_COMPARE(readHits(), 1);
        sqlite3_close(db);
    }

    void lowerLimitEvictsAtOnce()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        for (int i = 0; i < 10; i++)
        {
            _now += 60;
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000), 0));
        }
        cache->setLimit(5000);
        QCOMPARE(cache->getTotalBytes(), int64_t(3000));
        QVERIFY(cache->contains(key(9, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(0, 0, ZoomLevel15)));
    }

    void firstMapSyncKeepsNothingUnknown()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10), 0));
        // entries without a record of the maps they came from can not be trusted
        QCOMPARE(cache->syncMaps({ map("a.obf", tileArea(0, 0, 1)) }), 1);
        QCOMPARE(cache->getEntriesCount(), 0);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10), 0));
        QCOMPARE(cache->syncMaps({ map("a.obf", tileArea(0, 0, 1)) }), 0);
        QCOMPARE(cache->getEntriesCount(), 1);
    }

    // a map update removes exactly the tiles of its area, at every zoom
    void changedMapDropsItsArea()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        const auto nl = tileArea(2100, 1350, 12);       // one z12 tile
        const auto uk = tileArea(2040, 1360, 12);
        QCOMPARE(cache->syncMaps({ map("nl.obf", nl), map("uk.obf", uk) }), 0);
        QVERIFY(cache->put(key(2100, 1350, ZoomLevel12), blob(10), 0));            // the map tile
        QVERIFY(cache->put(key(2100 * 8 + 3, 1350 * 8 + 5, ZoomLevel15), blob(10), 0));  // inside it
        QVERIFY(cache->put(key(2100 / 8, 1350 / 8, ZoomLevel9), blob(10), 0));     // contains it
        QVERIFY(cache->put(key(2101, 1350, ZoomLevel12), blob(10), 0));            // the neighbour
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12), blob(10), 0));            // other map
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12, kDay, MapTileDiskCache::DataKind::Symbols), blob(10), 0));

        QCOMPARE(cache->syncMaps({ map("nl.obf", nl, 1000, 2), map("uk.obf", uk) }), 3);
        QVERIFY(!cache->contains(key(2100, 1350, ZoomLevel12)));
        QVERIFY(!cache->contains(key(2100 * 8 + 3, 1350 * 8 + 5, ZoomLevel15)));
        QVERIFY(!cache->contains(key(2100 / 8, 1350 / 8, ZoomLevel9)));
        QVERIFY(cache->contains(key(2101, 1350, ZoomLevel12)));
        QVERIFY(cache->contains(key(2040, 1360, ZoomLevel12)));
        QCOMPARE(cache->getEntriesCount(), 3);

        // same maps after a restart: nothing to do
        cache.reset();
        cache = open();
        QCOMPARE(cache->syncMaps({ map("nl.obf", nl, 1000, 2), map("uk.obf", uk) }), 0);
        QCOMPARE(cache->getEntriesCount(), 3);
    }

    void addedAndRemovedMapsDropTheirArea()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        const auto uk = tileArea(2040, 1360, 12);
        QCOMPARE(cache->syncMaps({ map("uk.obf", uk) }), 0);
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12), blob(10), 0));
        QVERIFY(cache->put(key(2100, 1350, ZoomLevel12), blob(10), 0));    // rendered before nl.obf was there
        QCOMPARE(cache->syncMaps({ map("uk.obf", uk), map("nl.obf", tileArea(2100, 1350, 12)) }), 1);
        QVERIFY(!cache->contains(key(2100, 1350, ZoomLevel12)));
        QVERIFY(cache->contains(key(2040, 1360, ZoomLevel12)));
        QCOMPARE(cache->syncMaps({ map("nl.obf", tileArea(2100, 1350, 12)) }), 1); // uk.obf deleted
        QCOMPARE(cache->getEntriesCount(), 0);
    }

    void worldMapUpdateDropsEverything()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        const AreaI world(0, 0, INT32_MAX, INT32_MAX);
        cache->syncMaps({ map("World_basemap.obf", world) });
        QVERIFY(cache->put(key(0, 0, ZoomLevel5), blob(10), 0));
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12), blob(10), 0));
        QCOMPARE(cache->syncMaps({ map("World_basemap.obf", world, 2000) }), 2);
    }

    // with two kept: day and night both stay, a third set of settings drops the oldest
    void renderingSettingsChange()
    {
        auto cache = open();
        cache->setKeptEnvironments(2);
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kDay), blob(10), 0));
        _now += 3600;
        cache->useEnvironment(kNight);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kNight), blob(10), 0));
        _now += 3600;
        cache->useEnvironment(kDay);
        QCOMPARE(cache->getEntriesCount(), 2);
        _now += 3600;
        cache->useEnvironment(kCar);
        QVERIFY(cache->contains(key(1, 1, ZoomLevel12, kDay)));
        QVERIFY(!cache->contains(key(1, 1, ZoomLevel12, kNight)));
        // a renderer with the dropped settings that is still finishing tiles does not refill the cache
        QVERIFY(!cache->put(key(2, 2, ZoomLevel12, kNight), blob(10), 0));

        // the choice survives a restart
        cache.reset();
        cache = open();
        QVERIFY(cache->put(key(3, 3, ZoomLevel12, kCar), blob(10), 0));
        QVERIFY(!cache->put(key(3, 3, ZoomLevel12, kNight), blob(10), 0));
    }

    // two renderers at once (phone + car screen) with a third set of settings do not evict each other;
    // once they are out of grace time they go even if no new settings come
    void recentlyUsedEnvironmentsAreNotDropped()
    {
        auto cache = open();
        cache->setKeptEnvironments(2);
        for (const uint64_t environment : { kDay, kNight, kCar })
        {
            cache->useEnvironment(environment);
            QVERIFY(cache->put(key(1, 1, ZoomLevel12, environment), blob(10), 0));
        }
        _now += 5 * 60;
        cache->useEnvironment(0x777);
        QCOMPARE(cache->getEntriesCount(), 3);
        _now += 6 * 60;
        cache->useEnvironment(kCar);   // only its use time is refreshed
        QVERIFY(!cache->contains(key(1, 1, ZoomLevel12, kDay)));
        QVERIFY(!cache->contains(key(1, 1, ZoomLevel12, kNight)));
        QVERIFY(cache->contains(key(1, 1, ZoomLevel12, kCar)));
    }

    void defaultKeepsFourEnvironments()
    {
        auto cache = open();
        for (const uint64_t environment : { kDay, kNight, kCar, uint64_t(0x777), uint64_t(0x888) })
        {
            _now += 3600;
            cache->useEnvironment(environment);
            QVERIFY(cache->put(key(1, 1, ZoomLevel12, environment), blob(10), 0));
        }
        QCOMPARE(cache->getEntriesCount(), 4);
        QVERIFY(!cache->contains(key(1, 1, ZoomLevel12, kDay)));
    }

    void damagedFileIsCreatedAgain()
    {
        {
            QFile f(path());
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(QByteArray(8192, '\x5A'));
        }
        auto cache = open();
        QVERIFY(cache->isOpen());
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10), 0));
        QCOMPARE(cache->getEntriesCount(), 1);
    }

    void olderFormatIsCreatedAgain()
    {
        {
            auto cache = open();
            cache->useEnvironment(kDay);
            QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10), 0));
        }
        {
            // pretend the file was written by another version: user_version is a big-endian int at offset 60
            QFile f(path());
            QVERIFY(f.open(QIODevice::ReadWrite));
            QVERIFY(f.seek(60));
            f.write(QByteArray("\x00\x00\x7F\xFF", 4));
        }
        auto cache = open();
        QVERIFY(cache->isOpen());
        QCOMPARE(cache->getEntriesCount(), 0);
    }

    void clearKeepsMapsAndEnvironments()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        cache->syncMaps({ map("a.obf", tileArea(0, 0, 1)) });
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10), 0));
        cache->clear();
        QCOMPARE(cache->getEntriesCount(), 0);
        QCOMPARE(cache->getTotalBytes(), int64_t(0));
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10), 0));
        QCOMPARE(cache->syncMaps({ map("a.obf", tileArea(0, 0, 1)) }), 0);
    }
};

QTEST_MAIN(TestMapTileDiskCache)
#include "TestMapTileDiskCache.moc"
