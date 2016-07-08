#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Search/AddressesByNameSearch.h>
#include <OsmAndCore/ObfsCollection.h>
#include <OsmAndCore/Utilities.h>

#include <QtTest/QtTest>
#include <QCoreApplication>

#include <memory>

using namespace OsmAnd;
using Criteria = AddressesByNameSearch::Criteria;
using ResultEntry = AddressesByNameSearch::ResultEntry;
Q_DECLARE_METATYPE(Criteria)
Q_DECLARE_METATYPE(ResultEntry)

class TestAddressSearch : public QObject
{
    Q_OBJECT

private slots:
    void search_data();
    void search();
};

void TestAddressSearch::search_data()
{
    QTest::addColumn<Criteria>("criteria");
    QTest::addColumn<QVector<ResultEntry>>("result");

    Criteria criteria;
    criteria.name = "Немига";
    QTest::newRow("улица Немига, Минск") << criteria << QVector<ResultEntry>();

    criteria.bbox31 = Utilities::boundingBox31FromLatLon(LatLon(53.9176, 27.5359), LatLon(53.8953, 27.5662));
    QTest::newRow("улица Немига, Минск with bbox") << criteria << QVector<ResultEntry>();
}

void TestAddressSearch::search()
{
    QFETCH(Criteria, criteria);
    QFETCH(QVector<ResultEntry>, result);
    auto obfs = std::make_shared<ObfsCollection>();
    obfs->addDirectory("/mnt/data_ssd/osmand/maps/belarus/");
    AddressesByNameSearch search{obfs};
    auto actual = search.performSearch(criteria);
    QCOMPARE(result, actual);
}

QTEST_MAIN(TestAddressSearch)
#include "TestAddressSearch.moc"
