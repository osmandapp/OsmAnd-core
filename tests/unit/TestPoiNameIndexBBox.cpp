#include <OsmAndCore.h>
#include <OsmAndCore/ICoreResourcesProvider.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/Data/Amenity.h>
#include <OsmAndCore/Data/ObfFile.h>
#include <OsmAndCore/Search/AmenitiesByNameSearch.h>
#include <OsmAndCore/Utilities.h>

#include <QtTest/QtTest>
#include <QCoreApplication>
#include <QFile>

#include <memory>

// Search by name must not lose a POI inside the request bbox only because the top-left corner of
// its name index data box is outside the bbox.
// Fixture: OsmAnd resources repo, test-resources/poi_name_index/README.md.
// OSMAND_TEST_POI_NAME_INDEX_OBF - path to poi_name_index_corner.obf,
// OSMAND_TEST_CORE_RESOURCES - directory with misc/icu4c/icu-data-l.dat.

using namespace OsmAnd;

namespace
{
    const uint64_t TARGET_ID = 2002;
    const uint64_t CONTROL_ID = 2004;

    class DirectoryResourcesProvider : public ICoreResourcesProvider
    {
    public:
        explicit DirectoryResourcesProvider(const QString& dir) : _dir(dir) {}

        QByteArray getResource(const QString& name, const float, bool* ok) const override
        {
            return getResource(name, ok);
        }

        QByteArray getResource(const QString& name, bool* ok) const override
        {
            QFile file(_dir + QLatin1Char('/') + name);
            const bool opened = file.open(QIODevice::ReadOnly);
            if (ok)
                *ok = opened;
            return opened ? file.readAll() : QByteArray();
        }

        bool containsResource(const QString& name, const float) const override
        {
            return containsResource(name);
        }

        bool containsResource(const QString& name) const override
        {
            return QFile::exists(_dir + QLatin1Char('/') + name);
        }

    private:
        const QString _dir;
    };
}

class TestPoiNameIndexBBox : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void smallBBoxFindsPoiOfDataBoxWithCornerOutside();
    void disjointBBoxDoesNotFindPoi();
    void wideBBoxFindsControlPoi();
    void smallBBoxAfterCacheWarmUp();

private:
    QList<uint64_t> search(const QString& name, const LatLon& topLeft, const LatLon& bottomRight) const;
    QList<uint64_t> searchSmall() const;

    std::shared_ptr<ObfsCollection> _obfs;
    std::shared_ptr<const ObfFile> _obfFile;
};

void TestPoiNameIndexBBox::initTestCase()
{
    const auto resources = qEnvironmentVariable("OSMAND_TEST_CORE_RESOURCES");
    const auto obf = qEnvironmentVariable("OSMAND_TEST_POI_NAME_INDEX_OBF");
    QVERIFY2(QFile::exists(obf), "set OSMAND_TEST_POI_NAME_INDEX_OBF");
    QVERIFY(InitializeCore(std::make_shared<DirectoryResourcesProvider>(resources)) != 0);
    // The file is searched directly: ObfsCollection::addFile without a directory has no index cache
    _obfs = std::make_shared<ObfsCollection>();
    _obfFile = std::make_shared<ObfFile>(obf);
}

void TestPoiNameIndexBBox::cleanupTestCase()
{
    _obfFile.reset();
    _obfs.reset();
    ReleaseCore();
}

void TestPoiNameIndexBBox::smallBBoxFindsPoiOfDataBoxWithCornerOutside()
{
    QCOMPARE(searchSmall(), QList<uint64_t>{ TARGET_ID });
}

void TestPoiNameIndexBBox::disjointBBoxDoesNotFindPoi()
{
    // This bbox lies in the control POI tile, east of the target data box.
    QCOMPARE(search("Cornerbox", LatLon(49.7230, 13.4000), LatLon(49.7224, 13.4010)), QList<uint64_t>{});
    QCOMPARE(search("Widebox", LatLon(49.7230, 13.4000), LatLon(49.7224, 13.4010)), QList<uint64_t>{ CONTROL_ID });
}

void TestPoiNameIndexBBox::wideBBoxFindsControlPoi()
{
    QCOMPARE(search("Cornerbox", LatLon(49.7250, 13.3920), LatLon(49.7200, 13.4010)), QList<uint64_t>{ TARGET_ID });
    QCOMPARE(search("Widebox", LatLon(49.7250, 13.3920), LatLon(49.7200, 13.4010)), QList<uint64_t>{ CONTROL_ID });
}

void TestPoiNameIndexBBox::smallBBoxAfterCacheWarmUp()
{
    search("Cornerbox", LatLon(49.7250, 13.3920), LatLon(49.7200, 13.4010));
    QCOMPARE(searchSmall(), QList<uint64_t>{ TARGET_ID });
    QCOMPARE(searchSmall(), QList<uint64_t>{ TARGET_ID });
}

QList<uint64_t> TestPoiNameIndexBBox::searchSmall() const
{
    return search("Cornerbox", LatLon(49.7215672, 13.3968384), LatLon(49.7201466, 13.3990356));
}

QList<uint64_t> TestPoiNameIndexBBox::search(const QString& name, const LatLon& topLeft, const LatLon& bottomRight) const
{
    AmenitiesByNameSearch::Criteria criteria;
    criteria.name = name;
    criteria.bbox31 = Utilities::boundingBox31FromLatLon(topLeft, bottomRight);
    QList<uint64_t> ids;
    AmenitiesByNameSearch search(_obfs);
    search.performSearchInFile(_obfFile, criteria,
        [&ids](const ISearch::Criteria&, const ISearch::IResultEntry& entry)
        {
            ids.append(static_cast<const AmenitiesByNameSearch::ResultEntry&>(entry).amenity->id.id);
        });
    return ids;
}

#if defined(OSMAND_TARGET_OS_android)
// A native test has no JVM, while Qt for Android finds standard paths through JNI.
// With Qt linked statically these replace Qt's own definitions.
QString QStandardPaths::writableLocation(StandardLocation)
{
    return QDir::tempPath();
}

QStringList QStandardPaths::standardLocations(StandardLocation)
{
    return { QDir::tempPath() };
}

QOperatingSystemVersion QOperatingSystemVersion::current()
{
    return QOperatingSystemVersion(QOperatingSystemVersion::Android, 0);
}
#endif

int main(int argc, char** argv)
{
    // Without it Qt for Android reads the version through JNI, and a native test has no JVM
    QCoreApplication::setApplicationVersion(QStringLiteral("test"));
    QCoreApplication app(argc, argv);
    TestPoiNameIndexBBox test;
    return QTest::qExec(&test, argc, argv);
}

#include "TestPoiNameIndexBBox.moc"
