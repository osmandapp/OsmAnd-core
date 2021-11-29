#include "MvtReader.h"
#include "MvtReader_P.h"

OsmAnd::MvtReader::MvtReader()
: _p(new MvtReader_P())
{
}

OsmAnd::MvtReader::~MvtReader()
{
}

uint8_t OsmAnd::MvtReader::getUserDataId(const std::string& key) 
{
    /*
     static const int CA_ID = 1;          // number  Camera heading. -1 if not found.
     static const int CAPTURED_AT_ID = 2; // number  When the image was captured, expressed as UTC epoch time in milliseconds. Must be non-negative integer; 0 if not found.
     static const int KEY_ID = 3;         // Key     Image key.
     static const int PANO_ID = 4;        // number  Whether the image is panorama (1), or not (0).
     static const int SKEY_ID = 5;        // Key     Sequence key.
     static const int USERKEY_ID = 6;
     */
    if (key == "compass_angle")
        return CA_ID;
    if (key == "captured_at")
        return CAPTURED_AT_ID;
    if (key == "id")
        return KEY_ID;
    if (key == "is_pano")
        return PANO_ID;
    if (key == "sequence_id")
        return SKEY_ID;
    if (key == "organization_id")
        return USERKEY_ID;
    
    return UNKNOWN_ID;
}

std::shared_ptr<const OsmAnd::MvtReader::Tile> OsmAnd::MvtReader::parseTile(const QString &pathToFile) const
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

QHash<uint8_t, QVariant> OsmAnd::MvtReader::Geometry::getUserData() const
{
    return _userData;
}

void OsmAnd::MvtReader::Geometry::setUserData(const QHash<uint8_t, QVariant> data)
{
    _userData = data;
}

OsmAnd::MvtReader::Point::Point(OsmAnd::PointI &coordinate)
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

OsmAnd::MvtReader::LineString::LineString(QVector<OsmAnd::PointI> &points)
: points(points)
{
}

OsmAnd::MvtReader::LineString::~LineString()
{
}

OsmAnd::MvtReader::GeomType OsmAnd::MvtReader::LineString::getType() const
{
    return LINE_STRING;
}

QVector<OsmAnd::PointI> OsmAnd::MvtReader::LineString::getCoordinateSequence() const
{
    return points;
}

OsmAnd::MvtReader::MultiLineString::MultiLineString(QVector<std::shared_ptr<const LineString>> &lines)
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

QVector<std::shared_ptr<const OsmAnd::MvtReader::LineString>> OsmAnd::MvtReader::MultiLineString::getLines() const
{
    return lines;
}

OsmAnd::MvtReader::Tile::Tile()
{
}

OsmAnd::MvtReader::Tile::~Tile()
{
}

int OsmAnd::MvtReader::Tile::addUserKey(const QString& userKey)
{
    int i = _userKeys.indexOf(userKey);
    if (i == -1)
    {
        _userKeys.append(userKey);
        return _userKeys.size() - 1;
    }
    else
    {
        return i;
    }
}

int OsmAnd::MvtReader::Tile::addSequenceKey(const QString& sequenceKey)
{
   int i = _sequenceKeys.indexOf(sequenceKey);
   if (i == -1)
   {
       _sequenceKeys.append(sequenceKey);
       return _sequenceKeys.size() - 1;
   }
   else
   {
       return i;
   }
}

QString OsmAnd::MvtReader::Tile::getUserKey(const uint32_t userKeyId) const
{
    return userKeyId < _userKeys.size() ? _userKeys.at(userKeyId) : QString();
}

QString OsmAnd::MvtReader::Tile::getSequenceKey(const uint32_t sequenceKeyId) const
{
    return sequenceKeyId < _sequenceKeys.size() ? _sequenceKeys.at(sequenceKeyId) : QString();
}

void OsmAnd::MvtReader::Tile::addGeometry(const std::shared_ptr<const Geometry> geometry)
{
    _geometry << geometry;
}

const bool OsmAnd::MvtReader::Tile::empty() const
{
    return _geometry.empty();
}

const QVector<std::shared_ptr<const OsmAnd::MvtReader::Geometry> > OsmAnd::MvtReader::Tile::getGeometry() const
{
    return _geometry;
}
