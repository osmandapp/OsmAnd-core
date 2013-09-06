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

#ifndef __OBF_READER_P_H_
#define __OBF_READER_P_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <QString>

#include <google/protobuf/io/coded_stream.h>

#include <OsmAndCore.h>
#include <OsmAndCore/IQueryController.h>

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
    class OSMAND_CORE_API ObfReader_P
    {
    private:
    protected:
        ObfReader_P(ObfReader* owner);

        ObfReader* const owner;
        std::shared_ptr<gpb::io::CodedInputStream> _codedInputStream;

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

#endif // __OBF_READER_P_H_

