#ifndef TESTSEARCHBYCOORDINATE_H
#define TESTSEARCHBYCOORDINATE_H

#include <QtTest/QtTest>

class TestSearchByCoordinate : public QObject
{
    Q_OBJECT

private slots:
    void search_data();
    void search();
};

#endif // TESTSEARCHBYCOORDINATE_H

