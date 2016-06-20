#include "SearchByCoordinate.h"

#include <QtTest/QtTest>
#include <QCoreApplication>

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
    QTest::addColumn<Point>("result");

    QTest::newRow("normal") << "12.34 56.78" << Point(12.34, 56.78);
    QTest::newRow("zeros") << "0.0 0.0" << Point(0.0, 0.0);
    QTest::newRow("without leading zero") << ".1 .2" << Point(0.1, 0.2);
    QTest::newRow("osm url") << "https://www.openstreetmap.org/#map=15/50.9307/4.7201" << Point(50.9307, 4.7201);
}

void TestSearchByCoordinate::search()
{
    QFETCH(QString, query);
    QFETCH(Point, result);
    QCOMPARE(SearchByCoordinate::search(query), result);
}

QTEST_MAIN(TestSearchByCoordinate)
#include "TestSearchByCoordinate.moc"
