#ifndef _OSMAND_CORE_MVT_READER_H_
#define _OSMAND_CORE_MVT_READER_H_

#include <OsmAndCore/stdlib_common.h>
#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QList>

#include <OsmAndCore/PrivateImplementation.h>

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
            QHash<QString, QString> userData;
            GeomType type;
        protected:
        public:
            Geometry();
            virtual ~Geometry();
            virtual GeomType getType() const;
            
            void setUserData(QHash<QString, QString> data);
            QHash<QString, QString> getUserData() const;
            
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
            QList<OsmAnd::PointI> points;
        protected:
        public:
            GeomType getType() const override;
            LineString(QList<OsmAnd::PointI> &points);
            virtual ~LineString();
            QList<OsmAnd::PointI> getCoordinateSequence() const;
        };
        
        class OSMAND_CORE_API MultiLineString : public Geometry
        {
            Q_DISABLE_COPY_AND_MOVE(MultiLineString);
        private:
            QList< std::shared_ptr<const LineString> > lines;
        protected:
        public:
            GeomType getType() const override;
            MultiLineString(QList< std::shared_ptr<const LineString> > &lines);
            virtual ~MultiLineString();
            QList< std::shared_ptr<const LineString> > getLines() const;
        };
        
        QList<std::shared_ptr<const Geometry> > parseTile(const QString &pathToFile) const;
            
    };
}

#endif // !defined(_OSMAND_CORE_MVT_READER_H_)
