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

#ifndef __OBF_READER_H_
#define __OBF_READER_H_

#include <OsmAndCore.h>
#include <QIODevice>
#include <memory>
#include <list>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <string>
#include <google/protobuf/io/coded_stream.h>
#include <ObfSection.h>
#include <ObfMapSection.h>
#include <ObfAddressRegionSection.h>
#include <ObfRoutingRegionSection.h>
#include <ObfPoiRegionSection.h>
#include <ObfTransportRegionSection.h>

namespace OsmAnd {

    namespace gpb = google::protobuf;

    /**
    OsmAnd Binary File reader
    */
    class OSMAND_CORE_API ObfReader
    {
    private:
        const std::shared_ptr<gpb::io::CodedInputStream> _codedInputStream;

        int _version;
        long _creationTimestamp;
        bool _isBasemap;
        std::list< std::shared_ptr<ObfMapSection> > _mapSections;
        std::list< std::shared_ptr<ObfAddressRegionSection> > _addressRegionsSections;
        std::list< std::shared_ptr<ObfRoutingRegionSection> > _routingRegionsSections;
        std::list< std::shared_ptr<ObfPoiRegionSection> > _poiRegionsSections;
        std::list< std::shared_ptr<ObfTransportRegionSection> > _transportSections;
    protected:
        static void skipUnknownField(gpb::io::CodedInputStream* cis, int tag);
    public:
        //! Constants
        enum {
            TransportStopZoom = 24,
            ShiftCoordinates = 5,
        };

        //! Ctor
        ObfReader(QIODevice* input);

    friend ObfMapSection;
    friend ObfAddressRegionSection;
    friend ObfRoutingRegionSection;
    friend ObfPoiRegionSection;
    friend ObfTransportRegionSection;
    };
} // namespace OsmAnd

#endif // __OBF_READER_H_
