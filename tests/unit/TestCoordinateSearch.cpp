#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Search/CoordinateSearch.h>

#include <QtTest/QtTest>
#include <QCoreApplication>

using namespace OsmAnd;
Q_DECLARE_METATYPE(LatLon)

class TestCoordinateSearch : public QObject
{
    Q_OBJECT

private:
    QString LatLonToQString(const LatLon &latlon);
private slots:
    void search_data();
    void search();
};

void TestCoordinateSearch::search_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<LatLon>("result");

    QTest::newRow("zeros")                                                            << "0.0 0.0"                                                                                                    << LatLon(0.0, 0.0);
    QTest::newRow("latlon int")                                                       << "12 56"                                                                                                      << LatLon(12, 56);
    QTest::newRow("latlon double")                                                    << "12.34 56.78"                                                                                                << LatLon(12.34, 56.78);
    QTest::newRow("latlon double without leading zero")                               << ".1 .2"                                                                                                      << LatLon(0.1, 0.2);
    QTest::newRow("latlon")                                                           << "83.62778 -32.66431"                                                                                         << LatLon(83.62778, -32.66431);
    QTest::newRow("latlon degrees letters")                                           << "N83d37.6668 W32d39.8586"                                                                                    << LatLon(83.62778, -32.66431);
    QTest::newRow("latlon minutes seconds")                                           << "83°37'40.008\"N 032°39'51.5\"W"                                                                             << LatLon(83.62778, -32.66431);
    QTest::newRow("latlon minutes seconds colon")                                     << "83:37:40.008 32:39:51.5\"W"                                                                                 << LatLon(83.62778, -32.66431);
    QTest::newRow("latlon minutes seconds single quotes separator")                   << "83'37'40.008 32'39'51.5\"W"                                                                                 << LatLon(83.62778, -32.66431);
    QTest::newRow("latlon minutes seconds double quotes separator")                   << "83\"37\"40.008 -32\"39\"51.5"                                                                               << LatLon(83.62778, -32.66431);
    QTest::newRow("MGRS")                                                             << "25XEN0415986552"                                                                                            << LatLon(83.62778, -32.66431);
    QTest::newRow("UTM/UPS")                                                          << "25n 504160 9286553"                                                                                         << LatLon(83.62778, -32.66427);
    QTest::newRow("geo int")                                                          << "geo:34,-106"                                                                                                << LatLon(34, -106);
    QTest::newRow("geo double")                                                       << "geo:34.99393,-106.61568"                                                                                    << LatLon(34.99393, -106.61568);
    QTest::newRow("geo double zoom")                                                  << "geo:34.99393,-106.61568?z=11"                                                                               << LatLon(34.99393, -106.61568);
    QTest::newRow("geo double name")                                                  << "geo:34.99393,-106.61568 (Treasure Island)"                                                                  << LatLon(34.99393, -106.61568);
    QTest::newRow("geo double zoom name")                                             << "geo:34.99393,-106.61568?z=11 (Treasure Island)"                                                             << LatLon(34.99393, -106.61568);
    QTest::newRow("geo double query name")                                            << "geo:34.99393,-106.61568?q=34.99393%2C-106.61568 (Treasure Island)"                                          << LatLon(34.99393, -106.61568);
    QTest::newRow("download.osmand.net")                                              << "http://download.osmand.net/go?lat=34&lon=-106&z=11"                                                         << LatLon(34, -106);
    QTest::newRow("osmand.net int")                                                   << "http://osmand.net/go?lat=34&lon=-106&z=11"                                                                  << LatLon(34, -106);
    QTest::newRow("osmand.net double")                                                << "http://www.osmand.net/go?lat=34.99393&lon=-106.61568&z=11"                                                  << LatLon(34.99393, -106.61568);
    QTest::newRow("openstreetmap.org")                                                << "https://www.openstreetmap.org/#map=15/50.9307/4.7201"                                                       << LatLon(50.9307, 4.7201);
    QTest::newRow("openstreetmap.org map int")                                        << "http://openstreetmap.org/#map=11/34/-106"                                                                   << LatLon(34, -106);
    QTest::newRow("openstreetmap.org map double")                                     << "http://openstreetmap.org/#map=11/34.99393/-106.61568"                                                       << LatLon(34.99393, -106.61568);
    QTest::newRow("openstreetmap.org zoom first")                                     << "http://openstreetmap.org/#11/34.99393/-106.61568"                                                           << LatLon(34.99393, -106.61568);
    QTest::newRow("openstreetmap.org marker")                                         << "https://www.openstreetmap.org/?mlat=34.993933029174805&mlon=-106.61568069458008#map=11/34.99393/-106.61568" << LatLon(34.99393, -106.61568);
    QTest::newRow("openstreetmap.org shortlink long double")                          << "http://osm.org/go/TyFYuF6P-?m"                                                                              << LatLon(34.99389, -106.6210);  // http://www.openstreetmap.org/?mlat=34.99390108510852&mlon=-106.62102732807398#map=19/34.99390/-106.62103
    QTest::newRow("openstreetmap.org shortlink double")                               << "http://osm.org/go/TyFS--"                                                                                   << LatLon(34.98047, -106.7871);  // http://www.openstreetmap.org/#map=3/34.98/-106.79
    QTest::newRow("openstreetmap.org shortlink long double with ~")                   << "http://osm.org/go/TyFYuF6P~~-?m"                                                                            << LatLon(34.99390, -106.6210);  // https://www.openstreetmap.org/?mlat=34.993933029174805&mlon=-106.61568069458008#map=15/34.99393/-106.61568
    QTest::newRow("openstreetmap.org shortlink long double deprecated format with @") << "http://osm.org/go/TyFYuF6P@@--?m"                                                                           << LatLon(34.99390, -106.6210);  // https://www.openstreetmap.org/?mlat=34.993933029174805&mlon=-106.61568069458008#map=15/34.99393/-106.61568
    QTest::newRow("maps.yandex.ru int")                                               << "http://maps.yandex.ru/?ll=34,-106&z=11"                                                                     << LatLon(34, -106);
    QTest::newRow("maps.yandex.ru double")                                            << "http://maps.yandex.ru/?ll=34.99393,-106.61568&z=11"                                                         << LatLon(34.99393, -106.61568);
    QTest::newRow("maps.google.com")                                                  << "http://maps.google.com/maps?q=N34.99393,E-106.6157"                                                         << LatLon(34.99393, -106.61568);
    QTest::newRow("maps.apple.com")                                                   << "http://maps.apple.com/?ll=34.99393,-106.61568"                                                              << LatLon(34.99393, -106.61568); // https://developer.apple.com/library/ios/featuredarticles/iPhoneURLScheme_Reference/MapLinks/MapLinks.html
    QTest::newRow("map.wap.qq.com")                                                   << "http://map.wap.qq.com/loc/detail.jsp?sid=ATBcz9TgirTZhWPWJtR3AhmD&g_ut=2&y=39.91082&x=116.48177"            << LatLon(39.91082, 116.48177);
    QTest::newRow("here.com")                                                         << "https://www.here.com/location?map=52.5134272,13.3778416,16,normal&msg=Hannah-Arendt-Stra%C3%9Fe"            << LatLon(52.5134272, 13.3778416);
    QTest::newRow("here.com")                                                         << "https://www.here.com/?map=48.23145,16.38454,15,normal"                                                      << LatLon(48.23145, 16.38454);
//            //

//            // http://map.qq.com/AppBox/print/?t=&c=%7B%22base%22%3A%7B%22l%22%3A11%2C%22lat%22%3A39.90403%2C%22lng%22%3A116.407526%7D%7D
//            z = 11;
//            url = "http://map.qq.com/AppBox/print/?t=&c=%7B%22base%22%3A%7B%22l%22%3A11%2C%22lat%22%3A" + dlat + "%2C%22lng%22%3A" + dlon + "%7D%7D";

//            // http://openstreetmap.de/zoom=11&lat=34&lon=-106
//            z = 11;
//            url = "http://openstreetmap.de/zoom=" + z + "&lat=" + ilat + "&lon=" + ilon;

//            // http://openstreetmap.de/zoom=11&lat=34.99393&lon=-106.61568
//            url = "http://openstreetmap.de/zoom=" + z + "&lat=" + dlat + "&lon=" + dlon;

//            // http://openstreetmap.de/lat=34.99393&lon=-106.61568&zoom=11
//            url = "http://openstreetmap.de/lat=" + dlat + "&lon=" + dlon + "&zoom=" + z;

//            // http://maps.google.com/maps/@34,-106,11z
//            url = "http://maps.google.com/maps/@" + ilat + "," + ilon + "," + z + "z";

//            // http://maps.google.com/maps/@34.99393,-106.61568,11z
//            url = "http://maps.google.com/maps/@" + dlat + "," + dlon + "," + z + "z";

//            // http://maps.google.com/maps/ll=34,-106,z=11
//            url = "http://maps.google.com/maps/ll=" + ilat + "," + ilon + ",z=" + z;

//            // http://maps.google.com/maps/ll=34.99393,-106.61568,z=11
//            url = "http://maps.google.com/maps/ll=" + dlat + "," + dlon + ",z=" + z;

//            // http://www.google.com/maps/?q=loc:34,-106&z=11
//            url = "http://www.google.com/maps/?q=loc:" + ilat + "," + ilon + "&z=" + z;

//            // http://www.google.com/maps/?q=loc:34.99393,-106.61568&z=11
//            url = "http://www.google.com/maps/?q=loc:" + dlat + "," + dlon + "&z=" + z;

//            // https://www.google.com/maps/preview#!q=paris&data=!4m15!2m14!1m13!1s0x47e66e1f06e2b70f%3A0x40b82c3688c9460!3m8!1m3!1d24383582!2d-95.677068!3d37.0625!3m2!1i1222!2i718!4f13.1!4m2!3d48.856614!4d2.3522219
//            url = "https://www.google.com/maps/preview#!q=paris&data=!4m15!2m14!1m13!1s0x47e66e1f06e2b70f%3A0x40b82c3688c9460!3m8!1m3!1d24383582!2d-95.677068!3d37.0625!3m2!1i1222!2i718!4f13.1!4m2!3d48.856614!4d2.3522219";

//            // TODO this URL does not work, where is it used?
//            // http://maps.google.com/maps/q=loc:34,-106&z=11
//            url = "http://maps.google.com/maps/q=loc:" + ilat + "," + ilon + "&z=" + z;

//            // TODO this URL does not work, where is it used?
//            // http://maps.google.com/maps/q=loc:34.99393,-106.61568&z=11
//            url = "http://maps.google.com/maps/q=loc:" + dlat + "," + dlon + "&z=" + z;

//            // TODO this URL does not work, where is it used?
//            // whatsapp
//            // http://maps.google.com/maps/q=loc:34,-106 (You)
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://maps.google.com/maps/q=loc:" + ilat + "," + ilon + " (You)";

//            // TODO this URL does not work, where is it used?
//            // whatsapp
//            // http://maps.google.com/maps/q=loc:34.99393,-106.61568 (You)
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://maps.google.com/maps/q=loc:" + dlat + "," + dlon + " (You)";

//            // whatsapp
//            // https://maps.google.com/maps?q=loc:34.99393,-106.61568 (You)
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "https://maps.google.com/maps?q=loc:" + dlat + "," + dlon + " (You)";

//            // whatsapp
//            // https://maps.google.com/maps?q=loc:34.99393,-106.61568 (USER NAME)
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "https://maps.google.com/maps?q=loc:" + dlat + "," + dlon + " (USER NAME)";

//            // whatsapp
//            // https://maps.google.com/maps?q=loc:34.99393,-106.61568 (USER NAME)
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "https://maps.google.com/maps?q=loc:" + dlat + "," + dlon + " (+55 99 99999-9999)";

//            // whatsapp
//            // https://www.google.com/maps/search/34.99393,-106.61568/data=!4m4!2m3!3m1!2s-23.2776,-45.8443128!4b1
//            url = "https://maps.google.com/maps?q=loc:" + dlat + "," + dlon + "/data=!4m4!2m3!3m1!2s-23.2776,-45.8443128!4b1";

//            // http://www.google.com/maps/search/food/34,-106,14z
//            url = "http://www.google.com/maps/search/food/" + ilat + "," + ilon + "," + z + "z";

//            // http://www.google.com/maps/search/food/34.99393,-106.61568,14z
//            url = "http://www.google.com/maps/search/food/" + dlat + "," + dlon + "," + z + "z";

//            // http://maps.google.com?saddr=Current+Location&daddr=34,-106
//            z = GeoParsedPoint.NO_ZOOM;

//            // http://maps.google.com?saddr=Current+Location&daddr=34.99393,-106.61568
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://maps.google.com?saddr=Current+Location&daddr=" + dlat + "," + dlon;

//            // http://www.google.com/maps/dir/Current+Location/34,-106
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://www.google.com/maps/dir/Current+Location/" + ilat + "," + ilon;

//            // http://www.google.com/maps/dir/Current+Location/34.99393,-106.61568
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://www.google.com/maps/dir/Current+Location/" + dlat + "," + dlon;

//            // http://maps.google.com/maps?q=34,-106
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://maps.google.com/maps?q=" + ilat + "," + ilon;

//            // http://maps.google.com/maps?q=34.99393,-106.61568
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://maps.google.com/maps?q=" + dlat + "," + dlon;

//            // http://maps.google.co.uk/?q=34.99393,-106.61568
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://maps.google.co.uk/?q=" + dlat + "," + dlon;

//            // http://www.google.com.tr/maps?q=34.99393,-106.61568
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://www.google.com.tr/maps?q=" + dlat + "," + dlon;

//            // http://maps.google.com/maps?lci=com.google.latitudepublicupdates&ll=34.99393%2C-106.61568&q=34.99393%2C-106.61568
//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://maps.google.com/maps?lci=com.google.latitudepublicupdates&ll=" + dlat
//                    + "%2C" + dlon + "&q=" + dlat + "%2C" + dlon + "((" + dlat + "%2C%20" + dlon + "))";

//            // https://www.google.com/maps/place/34%C2%B059'38.1%22N+106%C2%B036'56.5%22W/@34.99393,-106.61568,17z/data=!3m1!4b1!4m2!3m1!1s0x0:0x0
//            z = 17;
//            url = "https://www.google.com/maps/place/34%C2%B059'38.1%22N+106%C2%B036'56.5%22W/@" + dlat + "," + dlon + "," + z + "z/data=!3m1!4b1!4m2!3m1!1s0x0:0x0";

//            // http://map.baidu.com/?l=13&tn=B_NORMAL_MAP&c=13748138,4889173&s=gibberish
//            z = 7;
//            int latint = ((int) (dlat * 100000));
//            int lonint = ((int) (dlon * 100000));
//            url = "http://map.baidu.com/?l=" + z + "&tn=B_NORMAL_MAP&c=" + latint + "," + lonint + "&s=gibberish";

//            // http://www.amap.com/#!poi!!q=38.174596,114.995033|2|%E5%AE%BE%E9%A6%86&radius=1000
//            z = 13; // amap uses radius, so 1000m is roughly zoom level 13
//            url = "http://www.amap.com/#!poi!!q=" + dlat + "," + dlon + "|2|%E5%AE%BE%E9%A6%86&radius=1000";

//            z = GeoParsedPoint.NO_ZOOM;
//            url = "http://www.amap.com/?q=" + dlat + "," + dlon + ",%E4%B8%8A%E6%B5v%B7%E5%B8%82%E6%B5%A6%E4%B8%9C%E6%96%B0%E5%8C%BA%E4%BA%91%E5%8F%B0%E8%B7%AF8086";
//            System.out.println("\nurl: " + url);

//            // http://share.here.com/l/52.5134272,13.3778416,Hannah-Arendt-Stra%C3%9Fe?z=16.0&t=normal
//            url = "http://share.here.com/l/" + dlat + "," + dlon + ",Hannah-Arendt-Stra%C3%9Fe?z=" + z + "&t=normal";
//            System.out.println("url: " + url);

}

QString TestCoordinateSearch::LatLonToQString(LatLon const &latlon)
{
    return QString::number(latlon.latitude, 'g', 7) + " " + QString::number(latlon.longitude, 'g', 7);
}

void TestCoordinateSearch::search()
{
    QFETCH(QString, query);
    QFETCH(LatLon, result);
    LatLon actual = CoordinateSearch::search(query);
    QCOMPARE(LatLonToQString(actual), LatLonToQString(result));
}

QTEST_MAIN(TestCoordinateSearch)
#include "TestCoordinateSearch.moc"
