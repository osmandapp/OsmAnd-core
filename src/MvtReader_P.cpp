#include "MvtReader.h"
#include "MvtReader_P.h"

#include "QtCommon.h"
#include "QtExtensions.h"
#include <QString>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <QStandardPaths>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>

#include "QFileDeviceInputStream.h"
#include "QIODeviceInputStream.h"
#include "Utilities.h"
#include "Logging.h"

#define MIN_LINE_STRING_LEN 6

OsmAnd::MvtReader_P::MvtReader_P()
{
}

OsmAnd::MvtReader_P::~MvtReader_P()
{
}

std::shared_ptr<const OsmAnd::MvtReader::Tile> OsmAnd::MvtReader_P::parseTile(const QString &pathToFile) const
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    auto geometryTile = std::make_shared<OsmAnd::MvtReader::Tile>();
    
    std::string filename = pathToFile.toStdString();
    OsmAnd::VectorTile::Tile tile;
    
    const auto input = std::shared_ptr<QIODevice>(new QFile(pathToFile));
    // Create zero-copy input stream
    ::google::protobuf::io::ZeroCopyInputStream* zcis = nullptr;
    if (const auto inputFileDevice = std::dynamic_pointer_cast<QFileDevice>(input))
        zcis = new QFileDeviceInputStream(inputFileDevice);
    else
        zcis = new QIODeviceInputStream(input);
    
    const auto cis = new ::google::protobuf::io::CodedInputStream(zcis);
    cis->SetTotalBytesLimit(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    
    bool res = tile.MergeFromCodedStream(cis);
    delete cis;
    delete zcis;

    if (!res)
    {
        LogPrintf(OsmAnd::LogSeverityLevel::Debug, "Failed to parse protobuf");
        return geometryTile;
    }
    
    for (int i = 0; i < static_cast<std::size_t>(tile.layers_size()); ++i)
    {
        const auto& layer = tile.layers(i);
        
        for (int j = 0; j < static_cast<std::size_t>(layer.features_size()); ++j)
        {
            const auto& feature = layer.features(j);
            
            if (feature.type() == VectorTile::Tile_GeomType_UNKNOWN)
                continue;
            
            const auto& geomCmds = feature.geometry();
            readGeometry(geometryTile, geomCmds, feature, layer);
        }
    }

    return geometryTile;
}

QHash<uint8_t, QVariant> OsmAnd::MvtReader_P::parseUserData(const std::shared_ptr<OsmAnd::MvtReader::Tile>& geometryTile,
                                                            const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 > &tags,
                                                            const ::google::protobuf::RepeatedPtrField< ::std::string > &keys,
                                                            const google::protobuf::RepeatedPtrField< ::OsmAnd::VectorTile::Tile_Value > &values) const
{
    QHash<uint8_t, QVariant> result;
    uint32_t keyIndex, valIndex;
    for (int i = 0; i < tags.size() - 1; i += 2)
    {
        keyIndex = tags.Get(i);
        valIndex = tags.Get(i + 1);
        
        if (keyIndex >= 0 && keyIndex < keys.size() && valIndex >= 0 && valIndex < values.size())
        {
            auto userDataId = MvtReader::getUserDataId(keys.Get(keyIndex));
            if (userDataId != UNKNOWN_ID)
            {
                QVariant var = tileValueToVariant(values.Get(valIndex));
                if (userDataId == USERKEY_ID)
                    result[userDataId] = geometryTile->addUserKey(var.toString());
                else if (userDataId == SKEY_ID)
                     result[userDataId] = geometryTile->addSequenceKey(var.toString());
                else
                    result[userDataId] = var;
            }
        }
    }
    return result;
}

QVariant OsmAnd::MvtReader_P::tileValueToVariant(const OsmAnd::VectorTile::Tile_Value &value) const
{
    QVariant res;
    if (value.has_double_value())
        res = QVariant(value.double_value());
    else if (value.has_float_value())
        res = QVariant(value.float_value());
    else if (value.has_int_value())
        res = QVariant((qlonglong)value.int_value());
    else if (value.has_bool_value())
        res = QVariant(value.bool_value());
    else if (value.has_string_value())
        res = QVariant(value.string_value().c_str());
    else if (value.has_sint_value())
        res = QVariant((qlonglong)value.sint_value());
    else if (value.has_uint_value())
        res = QVariant((qulonglong)value.uint_value());

    return res;
}

void OsmAnd::MvtReader_P::readGeometry(const std::shared_ptr<OsmAnd::MvtReader::Tile>& geometryTile,
                                       const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry,
                                       const VectorTile::Tile_Feature& feature,
                                       const VectorTile::Tile_Layer& layer) const
{
    std::shared_ptr<OsmAnd::MvtReader::Geometry> geom = nullptr;
    switch (feature.type()) {
        case VectorTile::Tile_GeomType_POINT:
            geom = readPoint(geometry);
            break;
        case VectorTile::Tile_GeomType_LINESTRING:
            geom = readLineString(geometry);
            break;
//        case VectorTile::Tile_GeomType_POLYGON:
//            readMultiLineString(geometry, res);
//            break;
        default:
            break;
    }
    if (geom != nullptr)
    {
        geom->setUserData(parseUserData(geometryTile, feature.tags(), layer.keys(), layer.values()));
        geometryTile->addGeometry(geom);
    }
}

std::shared_ptr<OsmAnd::MvtReader::Geometry> OsmAnd::MvtReader_P::readPoint(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry) const
{
    if (geometry.size() == 0)
        return nullptr;
    
    /** Geometry command index */
    int i = 0;
    
    // Read command header
    const int cmdHdr = geometry.Get(i++);
    const int cmdLength = cmdHdr >> 3;
    const OsmAnd::CommandType cmd = getCommandType(cmdHdr);
    
    // Guard: command type
    if (cmd != SEG_MOVETO)
        return nullptr;
    
    // Guard: minimum command length
    if (cmdLength < 1)
        return nullptr;
    
    // Guard: header data unsupported by geometry command buffer
    //  (require header and at least 1 value * 2 params)
    if (cmdLength * 2 + 1 > geometry.size())
        return nullptr;
    
    OsmAnd::PointI nextCoord;
    
    while (i < geometry.size() - 1) {
        int x = zigZagDecode(geometry.Get(i++));
        int y = zigZagDecode(geometry.Get(i++));
        nextCoord = OsmAnd::PointI(x, y);
    }
    return std::make_shared<OsmAnd::MvtReader::Point>(nextCoord);
}

/**
 * See: <a href="https://developers.google.com/protocol-buffers/docs/encoding#types">Google Protocol Buffers Docs</a>
 *
 * @param n zig-zag encoded integer to decode
 * @return decoded integer
 */
int OsmAnd::MvtReader_P::zigZagDecode(const int &n) const
{
    return (n >> 1) ^ (-(n & 1));
}

OsmAnd::CommandType OsmAnd::MvtReader_P::getCommandType(const int &cmdHdr) const
{
    int cmdId = cmdHdr & 0x7;
    CommandType geomCmd;
    switch (cmdId) {
        case 1:
            geomCmd = SEG_MOVETO;
            break;
        case 2:
            geomCmd = SEG_LINETO;
            break;
        case 7:
            geomCmd = SEG_CLOSE;
            break;
        default:
            geomCmd = SEG_UNSUPPORTED;
    }
    return geomCmd;
    
}

std::shared_ptr<OsmAnd::MvtReader::Geometry> OsmAnd::MvtReader_P::readLineString(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry) const
{
    // Guard: must have header
    if (geometry.size() == 0)
        return nullptr;
    
    /** Geometry command index */
    int i = 0;
    

    OsmAnd::CommandType cmd;
    int cmdHdr;
    int cmdLength;
    QVector<std::shared_ptr<const OsmAnd::MvtReader::LineString>> geoms;
    
    int nextX = 0, nextY = 0;
    
    while (i <= geometry.size() - MIN_LINE_STRING_LEN) {
        // --------------------------------------------
        // Expected: MoveTo command of length 1
        // --------------------------------------------
        
        // Read command header
        cmdHdr = geometry.Get(i++);
        cmdLength = cmdHdr >> 3;
        cmd = getCommandType(cmdHdr);
        
        // Guard: command type and length
        if (cmd != SEG_MOVETO || cmdLength != 1)
            break;
        
        nextX += zigZagDecode(geometry.Get(i++));
        nextY += zigZagDecode(geometry.Get(i++));
        
        // --------------------------------------------
        // Expected: LineTo command of length > 0
        // --------------------------------------------
        
        // Read command header
        cmdHdr = geometry.Get(i++);
        cmdLength = cmdHdr >> 3;
        cmd = getCommandType(cmdHdr);
        
        // Guard: command type and length
        if (cmd != SEG_LINETO || cmdLength < 1)
            break;
        
        // Guard: header data length unsupported by geometry command buffer
        //  (require at least (1 value * 2 params) + current_index)
        if ((cmdLength * 2) + i > geometry.size())
            break;
        QVector<OsmAnd::PointI> points;
        points << OsmAnd::PointI(nextX, nextY);
        
        // Set remaining points from LineTo command
        for (int lineToIndex = 0; lineToIndex < cmdLength; ++lineToIndex) {
            nextX += zigZagDecode(geometry.Get(i++));
            nextY += zigZagDecode(geometry.Get(i++));
            points << OsmAnd::PointI(nextX, nextY);
        }

        geoms << std::make_shared<OsmAnd::MvtReader::LineString>(points);
    }
    
    if (geoms.count() == 1)
        return std::const_pointer_cast<OsmAnd::MvtReader::LineString>(geoms.first());
    else
        return std::make_shared<OsmAnd::MvtReader::MultiLineString>(geoms);
}
    

