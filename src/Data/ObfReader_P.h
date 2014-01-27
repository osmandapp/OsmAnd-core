/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#ifndef _OSMAND_CORE_OBF_READER_P_H_
#define _OSMAND_CORE_OBF_READER_P_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream.h>

#include <OsmAndCore.h>

class QIODevice;

namespace OsmAnd {

    namespace gpb = google::protobuf;

    class ObfInfo;

    class ObfMapSectionReader_P;
    class ObfAddressSectionReader_P;
    class ObfRoutingSectionReader_P;
    class ObfPoiSectionReader_P;
    class ObfTransportSectionReader_P;

    class ObfReader;
    class ObfReader_P
    {
    private:
    protected:
        ObfReader_P(ObfReader* owner);

        ObfReader* const owner;
        std::unique_ptr<gpb::io::CodedInputStream> _codedInputStream;
        std::unique_ptr<gpb::io::ZeroCopyInputStream> _zeroCopyInputStream;

        std::shared_ptr<QIODevice> _input;
        std::shared_ptr<ObfInfo> _obfInfo;

        QString transliterate(const QString& input);

        static void readInfo(const std::unique_ptr<ObfReader_P>& reader, const std::shared_ptr<ObfInfo>& info);
    public:
        virtual ~ObfReader_P();

    friend class OsmAnd::ObfReader;

    friend class OsmAnd::ObfMapSectionReader_P;
    friend class OsmAnd::ObfAddressSectionReader_P;
    friend class OsmAnd::ObfRoutingSectionReader_P;
    friend class OsmAnd::ObfPoiSectionReader_P;
    friend class OsmAnd::ObfTransportSectionReader_P;
    };
} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_READER_P_H_
