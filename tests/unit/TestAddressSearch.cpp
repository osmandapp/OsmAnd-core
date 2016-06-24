#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Search/CoordinateSearch.h>

#include <QtTest/QtTest>
#include <QCoreApplication>

using namespace OsmAnd;
Q_DECLARE_METATYPE(LatLon)

class TestAddressSearch : public QObject
{
    Q_OBJECT

private slots:
    void search_data();
    void search();
};

void TestAddressSearch::search_data()
{
    QTest::addColumn<QString>("query");
    QTest::addColumn<LatLon>("result");

    QTest::newRow("latlon int") << "12 56" << LatLon(12, 56);
}

void TestAddressSearch::search()
{
    QFETCH(QString, query);
    QFETCH(LatLon, result);
    LatLon actual = CoordinateSearch::search(query);
    QCOMPARE(result, actual);
}

QTEST_MAIN(TestAddressSearch)
#include "TestAddressSearch.moc"
