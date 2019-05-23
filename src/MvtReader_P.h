#ifndef _OSMAND_CORE_MVT_READER_P_H_
#define _OSMAND_CORE_MVT_READER_P_H_

#include <OsmAndCore/stdlib_common.h>

#include "ignore_warnings_on_external_includes.h"
#include "vector_tile.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>
#include <google/protobuf/wire_format_lite.h>
#include "restore_internal_warnings.h"

#include <OsmAndCore/QtExtensions.h>

namespace OsmAnd
{
    enum CommandType {
        SEG_UNSUPPORTED = -1,
        SEG_END    = 0,
        SEG_MOVETO = 1,
        SEG_LINETO = 2,
        SEG_CLOSE = (0x40 | 0x0f)
    };
    
    class OSMAND_CORE_API MvtReader_P
    {
        Q_DISABLE_COPY_AND_MOVE(MvtReader_P);
    public:
        QList<std::shared_ptr<const OsmAnd::MvtReader::Geometry> > parseTile(const QString &pathToFile) const;
    private:
        std::shared_ptr<const OsmAnd::MvtReader::Geometry> readGeometry(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry, VectorTile::Tile_GeomType type) const;
        
        QHash<QString, QString> parseUserData(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 > &tags,
                                              const ::google::protobuf::RepeatedPtrField< ::std::string> &keys,
                                              const google::protobuf::RepeatedPtrField<::OsmAnd::VectorTile::Tile_Value> &values) const;
        
        void readPoint(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry, std::shared_ptr<const OsmAnd::MvtReader::Geometry> &res) const;
        void readLineString(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry, std::shared_ptr<const OsmAnd::MvtReader::Geometry> &res) const;
        
        CommandType getCommandType(const int &cmdHdr) const;
        int zigZagDecode(const int &n) const;
        QString tileValueToString(const OsmAnd::VectorTile::Tile_Value &value) const;
    protected:
    public:
        MvtReader_P();
        virtual ~MvtReader_P();
    };
}

#endif // !defined(_OSMAND_CORE_MVT_READER_P_H_)
