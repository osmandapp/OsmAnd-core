#ifndef _OSMAND_CORE_MVT_READER_H_
#define _OSMAND_CORE_MVT_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QList>
#include <QVector>
#include <QVariant>

#include <OsmAndCore/PrivateImplementation.h>

static const int UNKNOWN_ID = 0;     // Unsupported id
static const int CA_ID = 1;          // number  Camera heading. -1 if not found.
static const int CAPTURED_AT_ID = 2; // number  When the image was captured, expressed as UTC epoch time in milliseconds. Must be non-negative integer; 0 if not found.
static const int KEY_ID = 3;         // Key     Image key.
static const int PANO_ID = 4;        // number  Whether the image is panorama (1), or not (0).
static const int SKEY_ID = 5;        // Key     Sequence key.
static const int USERKEY_ID = 6;     // Key     User key. Empty if not found.

namespace OsmAnd
{
    class MvtReader_P;
    class OSMAND_CORE_API MvtReader
    {
        Q_DISABLE_COPY_AND_MOVE(MvtReader);
    public:
        enum GeomType {
            UNKNOWN = 0,
            POINT = 1,
            LINE_STRING = 2,
            MULTI_LINE_STRING = 3,
            POLYGON = 4
        };
        
    private:
        PrivateImplementation<MvtReader_P> _p;
    protected:
    public:
        MvtReader();
        virtual ~MvtReader();
        
        class OSMAND_CORE_API Geometry
        {
            Q_DISABLE_COPY_AND_MOVE(Geometry);
            
        public:
        private:
            QHash<uint8_t, QVariant> _userData;
        protected:
        public:
            Geometry();
            virtual ~Geometry();
            virtual GeomType getType() const;
            
            void setUserData(const QHash<uint8_t, QVariant> data);
            QHash<uint8_t, QVariant> getUserData() const;
            
            friend class OsmAnd::MvtReader_P;
        };
        
        class OSMAND_CORE_API Point : public Geometry
        {
            Q_DISABLE_COPY_AND_MOVE(Point);
        private:
            OsmAnd::PointI point;
        protected:
        public:
            GeomType getType() const override;
            Point(OsmAnd::PointI &coordinates);
            virtual ~Point();
            OsmAnd::PointI getCoordinate() const;
        };
        
        class OSMAND_CORE_API LineString : public Geometry
        {
            Q_DISABLE_COPY_AND_MOVE(LineString);
        private:
            QVector<OsmAnd::PointI> points;
        protected:
        public:
            GeomType getType() const override;
            LineString(QVector<OsmAnd::PointI> &points);
            virtual ~LineString();
            QVector<OsmAnd::PointI> getCoordinateSequence() const;
        };
        
        class OSMAND_CORE_API MultiLineString : public Geometry
        {
            Q_DISABLE_COPY_AND_MOVE(MultiLineString);
        private:
            QVector< std::shared_ptr<const LineString> > lines;
        protected:
        public:
            GeomType getType() const override;
            MultiLineString(QVector< std::shared_ptr<const LineString> > &lines);
            virtual ~MultiLineString();
            QVector< std::shared_ptr<const LineString> > getLines() const;
        };
        
        class OSMAND_CORE_API Tile
        {
            Q_DISABLE_COPY_AND_MOVE(Tile);
            
        public:
        private:
            QStringList _userKeys;
            QStringList _sequenceKeys;
            QVector<std::shared_ptr<const Geometry> > _geometry;
            
            int addUserKey(const QString& userKey);
            int addSequenceKey(const QString& sequenceKey);
            void addGeometry(const std::shared_ptr<const Geometry> geometry);

        protected:
        public:
            Tile();
            virtual ~Tile();
            
            const bool empty() const;
            const QVector<std::shared_ptr<const Geometry> > getGeometry() const;
            QString getUserKey(const uint32_t userKeyId) const;
            QString getSequenceKey(const uint32_t sequenceKeyId) const;

            friend class OsmAnd::MvtReader_P;
        };
        
        static uint8_t getUserDataId(const std::string& key);
        std::shared_ptr<const Tile> parseTile(const QString &pathToFile) const;
    };
}

#endif // !defined(_OSMAND_CORE_MVT_READER_H_)
