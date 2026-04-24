#include "../../src/GeoTiffRasterWindow.h"

#include <QtTest/QtTest>

#include <limits>

using OsmAnd::GeoTiffRasterWindow::ClippedAxis;
using OsmAnd::GeoTiffRasterWindow::clipAxis;

class TestGeoTiffRasterWindow : public QObject
{
    Q_OBJECT

private slots:
    void keepsInteriorWindow();
    void clipsStartEdge();
    void clipsEndEdge();
    void clipsCornerWindow();
    void rejectsEmptySourceIntersection();
    void rejectsInvalidWindows();
};

void TestGeoTiffRasterWindow::keepsInteriorWindow()
{
    ClippedAxis axis;

    QVERIFY(clipAxis(10.0, 20.0, 100, 256, axis));
    QCOMPARE(axis.dataOffset, 10);
    QCOMPARE(axis.dataSize, 10);
    QCOMPARE(axis.tileOffset, 0);
    QCOMPARE(axis.tileLength, 256);
    QCOMPARE(axis.sourceStart, 10.0);
    QCOMPARE(axis.sourceEnd, 20.0);
}

void TestGeoTiffRasterWindow::clipsStartEdge()
{
    ClippedAxis axis;

    QVERIFY(clipAxis(-2.0, 6.0, 10, 8, axis));
    QCOMPARE(axis.dataOffset, 0);
    QCOMPARE(axis.dataSize, 6);
    QCOMPARE(axis.tileOffset, 2);
    QCOMPARE(axis.tileLength, 6);
    QCOMPARE(axis.sourceStart, 0.0);
    QCOMPARE(axis.sourceEnd, 6.0);
}

void TestGeoTiffRasterWindow::clipsEndEdge()
{
    ClippedAxis axis;

    QVERIFY(clipAxis(4.0, 12.0, 10, 8, axis));
    QCOMPARE(axis.dataOffset, 4);
    QCOMPARE(axis.dataSize, 6);
    QCOMPARE(axis.tileOffset, 0);
    QCOMPARE(axis.tileLength, 6);
    QCOMPARE(axis.sourceStart, 4.0);
    QCOMPARE(axis.sourceEnd, 10.0);
}

void TestGeoTiffRasterWindow::clipsCornerWindow()
{
    ClippedAxis xAxis;
    ClippedAxis yAxis;

    QVERIFY(clipAxis(-2.0, 6.0, 10, 8, xAxis));
    QVERIFY(clipAxis(4.0, 12.0, 10, 8, yAxis));

    QCOMPARE(xAxis.tileOffset, 2);
    QCOMPARE(xAxis.tileLength, 6);
    QCOMPARE(xAxis.tileOffset + xAxis.tileLength, 8);
    QCOMPARE(yAxis.tileOffset, 0);
    QCOMPARE(yAxis.tileLength, 6);
    QCOMPARE(yAxis.tileOffset + yAxis.tileLength, 6);
    QVERIFY(xAxis.dataOffset >= 0);
    QVERIFY(xAxis.dataOffset + xAxis.dataSize <= 10);
    QVERIFY(yAxis.dataOffset >= 0);
    QVERIFY(yAxis.dataOffset + yAxis.dataSize <= 10);
}

void TestGeoTiffRasterWindow::rejectsEmptySourceIntersection()
{
    ClippedAxis axis;

    QVERIFY(!clipAxis(-10.0, -2.0, 10, 8, axis));
    QVERIFY(!clipAxis(12.0, 20.0, 10, 8, axis));
}

void TestGeoTiffRasterWindow::rejectsInvalidWindows()
{
    ClippedAxis axis;
    const auto infinity = std::numeric_limits<double>::infinity();

    QVERIFY(!clipAxis(5.0, 5.0, 10, 8, axis));
    QVERIFY(!clipAxis(0.0, 10.0, 0, 8, axis));
    QVERIFY(!clipAxis(0.0, 10.0, 10, 0, axis));
    QVERIFY(!clipAxis(infinity, 10.0, 10, 8, axis));
}

QTEST_MAIN(TestGeoTiffRasterWindow)
#include "TestGeoTiffRasterWindow.moc"
