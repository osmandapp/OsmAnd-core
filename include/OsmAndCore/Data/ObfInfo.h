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

#ifndef _OSMAND_CORE_OBF_INFO_H_
#define _OSMAND_CORE_OBF_INFO_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>

class QIODevice;

namespace OsmAnd {

    class ObfReader;
    class ObfReader_P;

    class ObfMapSectionInfo;
    class ObfAddressSectionInfo;
    class ObfRoutingSectionInfo;
    class ObfPoiSectionInfo;
    class ObfTransportSectionInfo;

    class OSMAND_CORE_API ObfInfo
    {
        Q_DISABLE_COPY(ObfInfo)
    private:
    protected:
        ObfInfo();

        int _version;
        uint64_t _creationTimestamp;
        bool _isBasemap;

        QList< std::shared_ptr<ObfMapSectionInfo> > _mapSections;
        QList< std::shared_ptr<ObfAddressSectionInfo> > _addressSections;
        QList< std::shared_ptr<ObfRoutingSectionInfo> > _routingSections;
        QList< std::shared_ptr<ObfPoiSectionInfo> > _poiSections;
        QList< std::shared_ptr<ObfTransportSectionInfo> > _transportSections;
    public:
        virtual ~ObfInfo();

        const int& version;
        const uint64_t& creationTimestamp;
        const bool& isBasemap;

        const QList< std::shared_ptr<ObfMapSectionInfo> >& mapSections;
        const QList< std::shared_ptr<ObfAddressSectionInfo> >& addressSections;
        const QList< std::shared_ptr<ObfRoutingSectionInfo> >& routingSections;
        const QList< std::shared_ptr<ObfPoiSectionInfo> >& poiSections;
        const QList< std::shared_ptr<ObfTransportSectionInfo> >& transportSections;

    friend class OsmAnd::ObfReader;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_INFO_H_
