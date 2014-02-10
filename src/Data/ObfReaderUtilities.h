#ifndef _OSMAND_CORE_OBF_READER_UTILITIES_H_
#define _OSMAND_CORE_OBF_READER_UTILITIES_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QStringList>

#include <google/protobuf/io/coded_stream.h>

#include <OsmAndCore.h>

namespace OsmAnd {

    namespace ObfReaderUtilities {

        namespace gpb = google::protobuf;

        bool readQString(gpb::io::CodedInputStream* cis, QString& output);
        int32_t readSInt32(gpb::io::CodedInputStream* cis);
        int64_t readSInt64(gpb::io::CodedInputStream* cis);
        uint32_t readBigEndianInt(gpb::io::CodedInputStream* cis);
        void readStringTable(gpb::io::CodedInputStream* cis, QStringList& stringTableOut);
        void skipUnknownField(gpb::io::CodedInputStream* cis, int tag);
        QString encodeIntegerToString(const uint32_t value);
        uint32_t decodeIntegerFromString(const QString& container);

    } // namespace ObfReaderUtilities
    
} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_READER_UTILITIES_H_)

