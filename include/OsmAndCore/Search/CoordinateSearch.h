#ifndef SEARCHBYCOORDINATE_H
#define SEARCHBYCOORDINATE_H

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/LatLon.h>

#include <QString>
#include <QUrl>

namespace OsmAnd
{
    class OSMAND_CORE_API CoordinateSearch
    {
        Q_DISABLE_COPY_AND_MOVE(CoordinateSearch);

    public:
        static LatLon search(QString const &query);
    private:
        static QUrl toUrl(QString const &s);
        static QString withoutPrefix(QString const &query);
    };

}

#endif // SEARCHBYCOORDINATE_H
