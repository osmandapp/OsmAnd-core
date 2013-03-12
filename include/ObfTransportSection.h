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

#ifndef __OBF_TRANSPORT_REGION_SECTION_H_
#define __OBF_TRANSPORT_REGION_SECTION_H_

#include <OsmAndCore.h>
#include <memory>
#include <QList>
#include <string>
#include <google/protobuf/io/coded_stream.h>
#include <ObfSection.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    'Transport' section of OsmAnd Binary File
    */
    struct OSMAND_CORE_API ObfTransportSection : public ObfSection
    {
        ObfTransportSection(class ObfReader* owner);
        virtual ~ObfTransportSection();

        int _left;
        int _right;
        int _top;
        int _bottom;

        int _stopsFileOffset;
        int _stopsFileLength;
        enum {
            StopZoom = 24,
        };
    protected:
        static void read(ObfReader* reader, ObfTransportSection* section);

    private:
        static void readTransportBounds(ObfReader* reader, ObfTransportSection* section);

    friend class ObfReader;
    };

} // namespace OsmAnd

#endif // __OBF_TRANSPORT_REGION_SECTION_H_