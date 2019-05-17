#include "MvtReader.h"
#include "MvtReader_P.h"
#include "QtCommon.h"

#include "ignore_warnings_on_external_includes.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "restore_internal_warnings.h"

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QIODevice>
#include <OsmAndCore/Utilities.h>
#include <QStandardPaths>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <sstream>
#include "Logging.h"
#include "QtExtensions.h"

#define MIN_LINE_STRING_LEN 6

OsmAnd::MvtReader_P::MvtReader_P()
{
}

OsmAnd::MvtReader_P::~MvtReader_P()
{
}

QList<std::shared_ptr<const OsmAnd::MvtReader::Geometry> > OsmAnd::MvtReader_P::parseTile(const QString &pathToFile) const
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    
    std::string filename = pathToFile.toStdString();
    std::ifstream stream(filename.c_str(),std::ios_base::in|std::ios_base::binary);
    OsmAnd::VectorTile::Tile tile;
    std::string message(std::istreambuf_iterator<char>(stream.rdbuf()),(std::istreambuf_iterator<char>()));
    stream.close();
    QList<std::shared_ptr<const OsmAnd::MvtReader::Geometry>> result;
    if (!tile.ParseFromString(message))
    {
        std::clog << "failed to parse protobuf\n";
        return result;
    }
    for (int i = 0; i < static_cast<std::size_t>(tile.layers_size()); ++i)
    {
        const auto& layer = tile.layers(i);
        const auto& keys = layer.keys();
        
        for (int j = 0; j < static_cast<std::size_t>(layer.features_size()); ++j)
        {
            const auto& feature = layer.features(j);
            const uint64_t id = feature.has_id() ? feature.id() : -1;
            
            if (feature.type() == VectorTile::Tile_GeomType_UNKNOWN)
                continue;
            
            const auto& geomCmds = feature.geometry();
            result << readGeometry(geomCmds, feature.type());
        }
    }
    return result;
}

std::shared_ptr<OsmAnd::MvtReader::Geometry const> OsmAnd::MvtReader_P::readGeometry(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry, VectorTile::Tile_GeomType type) const
{
    std::shared_ptr<OsmAnd::MvtReader::Geometry const> res = nullptr;
    
    switch (type) {
        case VectorTile::Tile_GeomType_POINT:
            readPoint(geometry, res);
            break;
        case VectorTile::Tile_GeomType_LINESTRING:
            readLineString(geometry, res);
            break;
//        case VectorTile::Tile_GeomType_POLYGON:
//            readMultiLineString(geometry, res);
//            break;
        default:
            break;
    }
    
    return res;
}

void OsmAnd::MvtReader_P::readPoint(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry, std::shared_ptr<OsmAnd::MvtReader::Geometry const> &res) const
{
    if (geometry.size() == 0)
        return;
    
    /** Geometry command index */
    int i = 0;
    
    // Read command header
    const int cmdHdr = geometry.Get(i++);
    const int cmdLength = cmdHdr >> 3;
    const OsmAnd::CommandType cmd = getCommandType(cmdHdr);
    
    // Guard: command type
    if (cmd != SEG_MOVETO)
        return;
    
    // Guard: minimum command length
    if (cmdLength < 1)
        return;
    
    // Guard: header data unsupported by geometry command buffer
    //  (require header and at least 1 value * 2 params)
    if (cmdLength * 2 + 1 > geometry.size())
        return;
    
    OsmAnd::PointI nextCoord;
    
    while (i < geometry.size() - 1) {
        int x = zigZagDecode(geometry.Get(i++));
        int y = zigZagDecode(geometry.Get(i++));
        nextCoord = OsmAnd::PointI(x, y);
    }
    res.reset(new OsmAnd::MvtReader::Point(nextCoord));
}

/**
 * See: <a href="https://developers.google.com/protocol-buffers/docs/encoding#types">Google Protocol Buffers Docs</a>
 *
 * @param n zig-zag encoded integer to decode
 * @return decoded integer
 */
int OsmAnd::MvtReader_P::zigZagDecode(int n) const
{
    return (n >> 1) ^ (-(n & 1));
}

OsmAnd::CommandType OsmAnd::MvtReader_P::getCommandType(int cmdHdr) const
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

void OsmAnd::MvtReader_P::readLineString(const ::google::protobuf::RepeatedField< ::google::protobuf::uint32 >& geometry, std::shared_ptr<OsmAnd::MvtReader::Geometry const> &res) const
{
    // Guard: must have header
    if (geometry.size() == 0)
        return;
    
    /** Geometry command index */
    int i = 0;
    

    OsmAnd::CommandType cmd;
    int cmdHdr;
    int cmdLength;
    QList<std::shared_ptr<const OsmAnd::MvtReader::LineString>> geoms;
    QList<OsmAnd::PointI> points;
    
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
        
        int x = zigZagDecode(geometry.Get(i++));
        int y = zigZagDecode(geometry.Get(i++));
        
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
        
        points << OsmAnd::PointI(x, y);
        int nextX = x, nextY = y;
        // Set remaining points from LineTo command
        for (int lineToIndex = 0; lineToIndex < cmdLength; ++lineToIndex) {
            
            // Update cursor position with relative line delta
            nextX += zigZagDecode(geometry.Get(i++));
            nextY += zigZagDecode(geometry.Get(i++));
            points << OsmAnd::PointI(nextX, nextY);
        }
        
        geoms << std::make_shared<const OsmAnd::MvtReader::LineString>(points);
        points.clear();
    }
    
    if (geoms.count() == 1)
        res = geoms.first();
    else
        res.reset(new OsmAnd::MvtReader::MultiLineString(geoms));
}
    

