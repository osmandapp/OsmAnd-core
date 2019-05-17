#include "MvtReader.h"
#include "MvtReader_P.h"

OsmAnd::MvtReader::MvtReader()
: _p(new MvtReader_P())
{
}

OsmAnd::MvtReader::~MvtReader()
{
}

QList<std::shared_ptr<const OsmAnd::MvtReader::Geometry> > OsmAnd::MvtReader::parseTile(const QString &pathToFile) const
{
    return _p->parseTile(pathToFile);
}

OsmAnd::MvtReader::Geometry::Geometry()
{
}

OsmAnd::MvtReader::Geometry::~Geometry()
{
}

OsmAnd::MvtReader::GeomType OsmAnd::MvtReader::Geometry::getType() const
{
    return UNKNOWN;
}

QHash<QString, QString> OsmAnd::MvtReader::Geometry::getUserData() const
{
    return userData;
}

void OsmAnd::MvtReader::Geometry::setUserData(QHash<QString, QString> data)
{
    userData = data;
}

OsmAnd::MvtReader::Point::Point(OsmAnd::PointI coordinate)
: point(coordinate)
{
}

OsmAnd::MvtReader::Point::~Point()
{
}

OsmAnd::MvtReader::GeomType OsmAnd::MvtReader::Point::getType() const
{
    return POINT;
}

OsmAnd::PointI OsmAnd::MvtReader::Point::getCoordinate() const
{
    return point;
}

OsmAnd::MvtReader::LineString::LineString(QList<OsmAnd::PointI> &points)
: points(points)
{
}

OsmAnd::MvtReader::LineString::~LineString()
{
}

OsmAnd::MvtReader::GeomType OsmAnd::MvtReader::LineString::getType() const
{
    return  LINE_STRING;
}

QList<OsmAnd::PointI> OsmAnd::MvtReader::LineString::getCoordinateSequence() const
{
    return points;
}

OsmAnd::MvtReader::MultiLineString::MultiLineString(QList<std::shared_ptr<const LineString>> lines)
: lines(lines)
{
}

OsmAnd::MvtReader::MultiLineString::~MultiLineString()
{
}

OsmAnd::MvtReader::GeomType OsmAnd::MvtReader::MultiLineString::getType() const
{
    return MULTI_LINE_STRING;
}

QList<std::shared_ptr<const OsmAnd::MvtReader::LineString>> OsmAnd::MvtReader::MultiLineString::getLines() const
{
    return lines;
}


