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

#include <memory>
#include <functional>
#include <QIODevice>
#include <QList>
#include <QMultiHash>
#include <google/protobuf/io/coded_stream.h>
#include <OsmAndCore.h>
#include <ObfSection.h>
#include <ObfMapSection.h>
#include <ObfAddressSection.h>
#include <ObfRoutingSection.h>
#include <ObfPoiSection.h>
#include <ObfTransportSection.h>
#include <StreetGroup.h>
#include <IQueryController.h>

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
        QList< std::shared_ptr<ObfMapSection> > _mapSections;
        QList< std::shared_ptr<ObfAddressSection> > _addressSections;
        QList< std::shared_ptr<ObfRoutingSection> > _routingSections;
        QList< std::shared_ptr<ObfPoiSection> > _poiSections;
        QList< std::shared_ptr<ObfTransportSection> > _transportSections;
        QList< ObfSection* > _sections;
    protected:
        QString transliterate(QString input);
        static int readSInt32(gpb::io::CodedInputStream* cis);
        static int readBigEndianInt(gpb::io::CodedInputStream* cis);
        static void readStringTable(gpb::io::CodedInputStream* cis, QStringList& stringTableOut);
        static void skipUnknownField(gpb::io::CodedInputStream* cis, int tag);
    public:
        ObfReader(QIODevice* input);

        int getVersion() const;
        QList< OsmAnd::ObfSection* > getSections() const;

    friend class ObfData;
    friend struct ObfMapSection;
    friend struct ObfAddressSection;
    friend struct ObfRoutingSection;
    friend struct ObfPoiSection;
    friend struct ObfTransportSection;
    };
} // namespace OsmAnd

#endif // __OBF_READER_H_
