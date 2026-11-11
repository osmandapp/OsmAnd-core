#include <OsmAndCore/Map/MapTileDiskCache.h>

#include <QtTest/QtTest>
#include <QTemporaryDir>
#include <QFile>

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
            QVERIFY(cache->put(key(10, 20, ZoomLevel13), blob(100, 'a')));
            QVERIFY(cache->put(key(10, 20, ZoomLevel13, kDay, MapTileDiskCache::DataKind::Symbols), blob(50, 's')));
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
        QVERIFY(cache->put(key(1, 2, ZoomLevel10, kDay, MapTileDiskCache::DataKind::RasterImage, 10), blob(10)));
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
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(100)));
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(30, 'b')));
        QCOMPARE(cache->getEntriesCount(), 1);
        QCOMPARE(cache->getTotalBytes(), int64_t(30));
        QCOMPARE(cache->get(key(1, 1, ZoomLevel12)), blob(30, 'b'));
    }

    void onlyCachedZoomsAndSaneSizes()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        QVERIFY(!cache->put(key(1, 1, ZoomLevel16), blob(10)));
        QVERIFY(!cache->put(key(1, 1, ZoomLevel15), QByteArray()));
        QVERIFY(!cache->put(key(1, 1, ZoomLevel15), blob(1001)));   // more than 1/16 of the limit
        QVERIFY(cache->put(key(1, 1, ZoomLevel15), blob(1000)));
        QCOMPARE(cache->getEntriesCount(), 1);
    }

    void unknownEnvironmentIsNotStored()
    {
        auto cache = open();
        QVERIFY(!cache->put(key(1, 1, ZoomLevel12, kNight), blob(10)));
        cache->useEnvironment(kNight);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kNight), blob(10)));
    }

    // over the limit the entry idle the longest goes first, down to 90% of the limit
    void evictsLeastRecentlyUsed()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        for (int i = 0; i < 16; i++)
        {
            _now += 3600;
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000)));
        }
        QCOMPARE(cache->getEntriesCount(), 16);
        _now += 3600;
        QVERIFY(cache->put(key(16, 0, ZoomLevel15), blob(1000)));
        QVERIFY(cache->getTotalBytes() <= 16000 - 1600);
        QVERIFY(!cache->contains(key(0, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(1, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(2, 0, ZoomLevel15)));   // 17000 bytes -> under 14400
        QVERIFY(cache->contains(key(3, 0, ZoomLevel15)));
        QVERIFY(cache->contains(key(16, 0, ZoomLevel15)));
        QCOMPARE(cache->getEntriesCount(), 14);
    }

    // a hit refreshes the access time, so a tile that is still looked at survives
    void readKeepsEntryAlive()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        for (int i = 0; i < 16; i++)
        {
            _now += 3600;
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000)));
        }
        _now += 3600;
        QVERIFY(!cache->get(key(0, 0, ZoomLevel15)).isEmpty());
        _now += 3600;
        QVERIFY(cache->put(key(16, 0, ZoomLevel15), blob(1000)));
        QVERIFY(cache->contains(key(0, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(1, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(2, 0, ZoomLevel15)));
    }

    // z9-z14 live 4 times longer than z15, z12 8 times
    void zoomWeights()
    {
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel15), 1);
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel8), 1);
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel9), 4);
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel14), 4);
        QCOMPARE(MapTileDiskCache::getZoomWeight(ZoomLevel12), 8);

        auto cache = open(16000);
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(0, 0, ZoomLevel12), blob(1000)));    // idle 20 h at the end: 20 / 8 = 2.5
        QVERIFY(cache->put(key(0, 0, ZoomLevel11), blob(1000)));    // 20 / 4 = 5
        _now += 10 * 3600;
        QVERIFY(cache->put(key(0, 0, ZoomLevel15), blob(1000)));    // 10 / 1 = 10
        for (int i = 1; i < 14; i++)
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000)));
        _now += 10 * 3600;
        for (int i = 14; i < 16; i++)
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000)));
        QVERIFY(cache->contains(key(0, 0, ZoomLevel12)));
        QVERIFY(cache->contains(key(0, 0, ZoomLevel11)));
        QVERIFY(!cache->contains(key(0, 0, ZoomLevel15)));
    }

    void lowerLimitEvictsAtOnce()
    {
        auto cache = open(16000);
        cache->useEnvironment(kDay);
        for (int i = 0; i < 10; i++)
        {
            _now += 60;
            QVERIFY(cache->put(key(i, 0, ZoomLevel15), blob(1000)));
        }
        cache->setLimit(5000);
        QVERIFY(cache->getTotalBytes() <= 5000);
        QVERIFY(cache->contains(key(9, 0, ZoomLevel15)));
        QVERIFY(!cache->contains(key(0, 0, ZoomLevel15)));
    }

    void firstMapSyncKeepsNothingUnknown()
    {
        auto cache = open();
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10)));
        // entries without a record of the maps they came from can not be trusted
        QCOMPARE(cache->syncMaps({ map("a.obf", tileArea(0, 0, 1)) }), 1);
        QCOMPARE(cache->getEntriesCount(), 0);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10)));
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
        QVERIFY(cache->put(key(2100, 1350, ZoomLevel12), blob(10)));            // the map tile
        QVERIFY(cache->put(key(2100 * 8 + 3, 1350 * 8 + 5, ZoomLevel15), blob(10)));  // inside it
        QVERIFY(cache->put(key(2100 / 8, 1350 / 8, ZoomLevel9), blob(10)));     // contains it
        QVERIFY(cache->put(key(2101, 1350, ZoomLevel12), blob(10)));            // the neighbour
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12), blob(10)));            // other map
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12, kDay, MapTileDiskCache::DataKind::Symbols), blob(10)));

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
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12), blob(10)));
        QVERIFY(cache->put(key(2100, 1350, ZoomLevel12), blob(10)));    // rendered before nl.obf was there
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
        QVERIFY(cache->put(key(0, 0, ZoomLevel5), blob(10)));
        QVERIFY(cache->put(key(2040, 1360, ZoomLevel12), blob(10)));
        QCOMPARE(cache->syncMaps({ map("World_basemap.obf", world, 2000) }), 2);
    }

    // with two kept: day and night both stay, a third set of settings drops the oldest
    void renderingSettingsChange()
    {
        auto cache = open();
        cache->setKeptEnvironments(2);
        cache->useEnvironment(kDay);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kDay), blob(10)));
        _now += 3600;
        cache->useEnvironment(kNight);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kNight), blob(10)));
        _now += 3600;
        cache->useEnvironment(kDay);
        QCOMPARE(cache->getEntriesCount(), 2);
        _now += 3600;
        cache->useEnvironment(kCar);
        QVERIFY(cache->contains(key(1, 1, ZoomLevel12, kDay)));
        QVERIFY(!cache->contains(key(1, 1, ZoomLevel12, kNight)));
        // a renderer with the dropped settings that is still finishing tiles does not refill the cache
        QVERIFY(!cache->put(key(2, 2, ZoomLevel12, kNight), blob(10)));

        // the choice survives a restart
        cache.reset();
        cache = open();
        QVERIFY(cache->put(key(3, 3, ZoomLevel12, kCar), blob(10)));
        QVERIFY(!cache->put(key(3, 3, ZoomLevel12, kNight), blob(10)));
    }

    // two renderers at once (phone + car screen) with a third set of settings do not evict each other
    void recentlyUsedEnvironmentsAreNotDropped()
    {
        auto cache = open();
        cache->setKeptEnvironments(2);
        cache->useEnvironment(kDay);
        cache->useEnvironment(kNight);
        cache->useEnvironment(kCar);
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kDay), blob(10)));
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kNight), blob(10)));
        QVERIFY(cache->put(key(1, 1, ZoomLevel12, kCar), blob(10)));
        QCOMPARE(cache->getEntriesCount(), 3);
        _now += 3600;
        cache->useEnvironment(kCar);
        cache->useEnvironment(kNight);
        cache->useEnvironment(0x777);
        QVERIFY(!cache->contains(key(1, 1, ZoomLevel12, kDay)));
        QVERIFY(cache->contains(key(1, 1, ZoomLevel12, kCar)));
        QVERIFY(cache->contains(key(1, 1, ZoomLevel12, kNight)));
    }

    void defaultKeepsFourEnvironments()
    {
        auto cache = open();
        for (const uint64_t environment : { kDay, kNight, kCar, uint64_t(0x777), uint64_t(0x888) })
        {
            _now += 3600;
            cache->useEnvironment(environment);
            QVERIFY(cache->put(key(1, 1, ZoomLevel12, environment), blob(10)));
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
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10)));
        QCOMPARE(cache->getEntriesCount(), 1);
    }

    void olderFormatIsCreatedAgain()
    {
        {
            auto cache = open();
            cache->useEnvironment(kDay);
            QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10)));
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
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10)));
        cache->clear();
        QCOMPARE(cache->getEntriesCount(), 0);
        QCOMPARE(cache->getTotalBytes(), int64_t(0));
        QVERIFY(cache->put(key(1, 1, ZoomLevel12), blob(10)));
        QCOMPARE(cache->syncMaps({ map("a.obf", tileArea(0, 0, 1)) }), 0);
    }
};

QTEST_MAIN(TestMapTileDiskCache)
#include "TestMapTileDiskCache.moc"
