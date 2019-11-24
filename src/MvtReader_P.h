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
        std::shared_ptr<const OsmAnd::MvtReader::Tile> parseTile(const QString &pathToFile) const;
    private:
        void readGeometry(const std::shared_ptr<OsmAnd::MvtReader::Tile>& geometryTile,
                          const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry,
                          const VectorTile::Tile_Feature& feature,
                          const VectorTile::Tile_Layer& layer) const;
        
        QHash<uint8_t, QVariant> parseUserData(const std::shared_ptr<OsmAnd::MvtReader::Tile>& geometryTile,
                                               const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 > &tags,
                                               const ::google::protobuf::RepeatedPtrField< ::std::string> &keys,
                                               const google::protobuf::RepeatedPtrField<::OsmAnd::VectorTile::Tile_Value> &values) const;
        
        std::shared_ptr<OsmAnd::MvtReader::Geometry> readPoint(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry) const;
        std::shared_ptr<OsmAnd::MvtReader::Geometry> readLineString(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry) const;
        
        CommandType getCommandType(const int &cmdHdr) const;
        int zigZagDecode(const int &n) const;
        QVariant tileValueToVariant(const OsmAnd::VectorTile::Tile_Value &value) const;
    protected:
    public:
        MvtReader_P();
        virtual ~MvtReader_P();
    };
}

#endif // !defined(_OSMAND_CORE_MVT_READER_P_H_)
