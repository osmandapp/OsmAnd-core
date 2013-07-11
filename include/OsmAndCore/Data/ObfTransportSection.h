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

#ifndef __OBF_TRANSPORT_SECTION_H_
#define __OBF_TRANSPORT_SECTION_H_

#include <memory>
#include <string>

#include <QList>

#include <google/protobuf/io/coded_stream.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Data/ObfSection.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    class ObfReader;
    namespace gpb = google::protobuf;

    /**
    'Transport' section of OsmAnd Binary File
    */
    class OSMAND_CORE_API ObfTransportSection : public ObfSection
    {
    private:
        static void readTransportBounds(ObfReader* reader, ObfTransportSection* section);
    protected:
        ObfTransportSection(class ObfReader* owner);

        AreaI _area24;

        int _stopsFileOffset;
        int _stopsFileLength;
        static void read(ObfReader* reader, ObfTransportSection* section);
    public:
        virtual ~ObfTransportSection();

        enum {
            StopZoom = 24,
        };

        const AreaI& area24;
    friend class OsmAnd::ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_TRANSPORT_SECTION_H_