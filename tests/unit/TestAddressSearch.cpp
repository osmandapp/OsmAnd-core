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

    // Add data here
}

void TestAddressSearch::search()
{
//    QFETCH(QString, query);
//    QCOMPARE(result, actual);
}

QTEST_MAIN(TestAddressSearch)
#include "TestAddressSearch.moc"
