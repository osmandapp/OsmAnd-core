/**
 * @file
 *
 * @section LICENSE
 *
 * OsmAnd - Android navigation software based on OSM maps.
 * Copyright (C) 2010-2013  OsmAnd Authors listed in AUTHORS file
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __OBF_READER_UTILITIES_H_
#define __OBF_READER_UTILITIES_H_

#include <stdint.h>

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
        QString encodeIntegerToString(const uint32_t& value);
        uint32_t decodeIntegerFromString(const QString& container);

    } // namespace ObfReaderUtilities
    
} // namespace OsmAnd

#endif // __OBF_READER_UTILITIES_H_

