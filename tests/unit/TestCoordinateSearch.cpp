#include "SearchByCoordinate.h"

#include <OsmAndCore/LatLon.h>

#include <QtTest/QtTest>
#include <QCoreApplication>

#include <type_traits>

using namespace OsmAnd;
Q_DECLARE_METATYPE(LatLon)

class TestSearchByCoordinate : public QObject
{
    Q_OBJECT

private slots:
    void search_data();
    void search();
};

void TestSearchByCoordinate::search_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<LatLon>("result");

    QTest::newRow("latlon int") << "12 56" << LatLon(12, 56);
    QTest::newRow("latlon double") << "12.34 56.78" << LatLon(12.34, 56.78);
    QTest::newRow("zeros") << "0.0 0.0" << LatLon(0.0, 0.0);
    QTest::newRow("without leading zero") << ".1 .2" << LatLon(0.1, 0.2);
    QTest::newRow("osm url") << "https://www.openstreetmap.org/#map=15/50.9307/4.7201" << LatLon(50.9307, 4.7201);

    // Geo intent tests
    int const iLat = 34, iLon = -106;
    double const dLat = 34.99393, dLon = -106.61568;
    double const longLat = 34.993933029174805, longLon = -106.615680694580078;
    int const z = 11;

    QString const siLat = QString::number(iLat), siLon = QString::number(iLon);
    QString const sdLat = QString::number(dLat, 'g', 7), sdLon = QString::number(dLon, 'g', 8);
//    QString const sLongLat = QString::number(longLat, 'g', 17), sLongLon = QString::number(longLon, 'g', 18);

    QString const name = "Treasure Island";

    // geo:34,-106
    QTest::newRow("geo int") << "geo:" + siLat + "," +  siLon << LatLon(iLat, iLon);

    // geo:34.99393,-106.61568
    QTest::newRow("geo double") << "geo:" + sdLat + "," + sdLon << LatLon(dLat, dLon);

    // geo:34.99393,-106.61568?z=11
    QTest::newRow("geo double zoom") << "geo:" + sdLat + "," + sdLon + "?z=" + z << LatLon(dLat, dLon);

    // geo:34.99393,-106.61568 (Treasure Island)
    QTest::newRow("geo double name") << "geo:" + sdLat + "," + sdLon + " (" + name + ")" << LatLon(dLat, dLon);

    // geo:34.99393,-106.61568?z=11 (Treasure Island)
    QTest::newRow("geo double zoom name") << "geo:" + sdLat + "," + sdLon + "?z=" + z + " (" + name + ")" << LatLon(dLat, dLon);

    // geo:34.99393,-106.61568?q=34.99393%2C-106.61568 (Treasure Island)
    QTest::newRow("geo double query name") << "geo:" + sdLat + "," + sdLon + "?z=" + z + " (" + name + ")" << LatLon(dLat, dLon);


    // Url tests
    // http://download.osmand.net/go?lat=34&lon=-106&z=11
    QTest::newRow("download.osmand.net") << "http://download.osmand.net/go?lat=" + siLat + "&lon=" + siLon + "&z=" + z << LatLon(iLat, iLon);

    // http://osmand.net/go?lat=34&lon=-106&z=11
    QTest::newRow("osmand.net int") << "http://www.osmand.net/go.html?lat=" + siLat + "&lon=" + siLon + "&z=" + z << LatLon(iLat, iLon);

    // http://www.osmand.net/go?lat=34.99393&lon=-106.61568&z=11
    QTest::newRow("osmand.net double") << "http://www.osmand.net/go.html?lat=" + sdLat + "&lon=" + sdLon + "&z=" + z << LatLon(dLat, dLon);

    // http://maps.google.com/maps?q=N34.939,E-106
    QTest::newRow("maps.google.com") << "http://maps.google.com/maps?q=N" + sdLat + ",E" + siLon << LatLon(dLat, iLon);


    // Various coordinate systems testing
    // Taken from http://geographiclib.sourceforge.net/cgi-bin/GeoConvert
//    double lat = 83.627, lon = -32.664;  // 83.62778, -32.66434
    LatLon latlon = LatLon(83.62778, -32.66431);
    QTest::newRow("latlon") << "83.62778 -32.66431" << latlon;
    QTest::newRow("latlon degrees letters") << "N83d37.6668 W32d39.8586" << latlon;
    QTest::newRow("latlon hours seconds") << "83°37'40.0\"N 032°39'51.5\"W" << latlon;
    QTest::newRow("latlon hours seconds colon") << "83:37:40.0 32:39:51.5\"W" << latlon;
    QTest::newRow("MGRS") << "25XEN0415986552" << latlon;
    QTest::newRow("UTM/UPS") << "25n 504160 9286553" << latlon;
}

void TestSearchByCoordinate::search()
{
    QFETCH(QString, query);
    QFETCH(LatLon, result);
    LatLon actual = CoordinateSearch::search(query);
    QCOMPARE(actual, result);
}

QTEST_MAIN(TestSearchByCoordinate)
#include "TestSearchByCoordinate.moc"
