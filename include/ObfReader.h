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
//#include <unicode/translit.h>
#include <google/protobuf/io/coded_stream.h>
#include <ObfSection.h>
#include <ObfMapSection.h>
#include <ObfAddressSection.h>
#include <ObfRoutingSection.h>
#include <ObfPoiSection.h>
#include <ObfTransportSection.h>
#include <StreetGroup.h>

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
        std::list< std::shared_ptr<ObfAddressSection> > _addressRegionsSections;
        std::list< std::shared_ptr<ObfRoutingSection> > _routingRegionsSections;
        std::list< std::shared_ptr<ObfPoiSection> > _poiRegionsSections;
        std::list< std::shared_ptr<ObfTransportSection> > _transportSections;
        std::list< ObfSection* > _sections;
        //std::shared_ptr<Transliterator> _icuTransliterator;
    protected:
        QString transliterate(QString input);
        static int readSInt32(gpb::io::CodedInputStream* cis);
        static int readBigEndianInt(gpb::io::CodedInputStream* cis);
        static void skipUnknownField(gpb::io::CodedInputStream* cis, int tag);
    public:
        //! Constants
        enum {
            //TODO: move to transport?
            TransportStopZoom = 24,
            ShiftCoordinates = 5,
        };

        ObfReader(QIODevice* input);

        int getVersion() const;
        std::list< OsmAnd::ObfSection* > getSections() const;

    friend struct ObfMapSection;
    friend struct ObfAddressSection;
    friend struct ObfRoutingSection;
    friend struct ObfPoiSection;
    friend struct ObfTransportSection;
    };
} // namespace OsmAnd

#endif // __OBF_READER_H_
