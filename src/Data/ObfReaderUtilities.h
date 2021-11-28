#ifndef _OSMAND_CORE_OBF_READER_UTILITIES_H_
#define _OSMAND_CORE_OBF_READER_UTILITIES_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QString>
#include <QStringList>
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <google/protobuf/io/coded_stream.h>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PointsAndAreas.h"

namespace OsmAnd
{
    class ObfSectionInfo;
    class ObfReader;

    namespace gpb = google::protobuf;

    struct ObfReaderUtilities Q_DECL_FINAL
    {
        static bool readQString(gpb::io::CodedInputStream* cis, QString& output);
        static int32_t readSInt32(gpb::io::CodedInputStream* cis);
        static int64_t readSInt64(gpb::io::CodedInputStream* cis);
        static uint32_t readBigEndianInt(gpb::io::CodedInputStream* cis);
        static uint32_t readLength(gpb::io::CodedInputStream* cis);
        static void readStringTable(gpb::io::CodedInputStream* cis, QStringList& stringTableOut);
        static int scanIndexedStringTable(
            gpb::io::CodedInputStream* cis,
            const QString& query,
            QVector<uint32_t>& outValues,
            const bool strictMatch = false,
            const QString& keysPrefix = {},
            const int matchedCharactersCount = 0);
        static void readTileBox(gpb::io::CodedInputStream* cis, AreaI& outArea);

        static void skipUnknownField(gpb::io::CodedInputStream* cis, int tag);
        static void skipBlockWithLength(gpb::io::CodedInputStream* cis);

        static QString encodeIntegerToString(const uint32_t value);
        static uint32_t decodeIntegerFromString(const QString& container);

        static bool reachedDataEnd(gpb::io::CodedInputStream* cis);
        static void ensureAllDataWasRead(gpb::io::CodedInputStream* cis);
    };
}

#endif // !defined(_OSMAND_CORE_OBF_READER_UTILITIES_H_)
