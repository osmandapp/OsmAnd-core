#ifndef SEARCHBYCOORDINATE_H
#define SEARCHBYCOORDINATE_H

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/LatLon.h>

#include <QString>
#include <QUrl>

namespace OsmAnd
{

    class CoordinateSearch
    {
    public:
        static LatLon search(QString query);
    private:
        static QUrl toUrl(QString s);
        static QString withoutPrefix(QString query);
    };

}

#endif // SEARCHBYCOORDINATE_H
