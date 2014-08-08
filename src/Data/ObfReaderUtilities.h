#ifndef _OSMAND_CORE_OBF_READER_UTILITIES_H_
#define _OSMAND_CORE_OBF_READER_UTILITIES_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QString>
#include <QStringList>

#include "ignore_warnings_on_external_includes.h"
#include <google/protobuf/io/coded_stream.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"

namespace OsmAnd
{
    namespace ObfReaderUtilities
    {
        namespace gpb = google::protobuf;

        bool readQString(gpb::io::CodedInputStream* cis, QString& output);
        int32_t readSInt32(gpb::io::CodedInputStream* cis);
        int64_t readSInt64(gpb::io::CodedInputStream* cis);
        uint32_t readBigEndianInt(gpb::io::CodedInputStream* cis);
        uint32_t readLength(gpb::io::CodedInputStream* cis);
        void readStringTable(gpb::io::CodedInputStream* cis, QStringList& stringTableOut);
        void skipUnknownField(gpb::io::CodedInputStream* cis, int tag);
        void skipBlockWithLength(gpb::io::CodedInputStream* cis);
        QString encodeIntegerToString(const uint32_t value);
        uint32_t decodeIntegerFromString(const QString& container);
    }
}

#endif // !defined(_OSMAND_CORE_OBF_READER_UTILITIES_H_)

