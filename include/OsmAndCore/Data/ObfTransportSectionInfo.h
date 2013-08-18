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

#ifndef __OBF_TRANSPORT_SECTION_INFO_H_
#define __OBF_TRANSPORT_SECTION_INFO_H_

#include <stdint.h>
#include <memory>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfTransportSectionReader_P;
    class ObfReader_P;

    class OSMAND_CORE_API ObfTransportSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfTransportSectionInfo)
    private:
    protected:
        ObfTransportSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        AreaI _area24;

        uint32_t _stopsOffset;
        uint32_t _stopsLength;
    public:
        virtual ~ObfTransportSectionInfo();

        const AreaI& area24;

        friend class OsmAnd::ObfTransportSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // __OBF_TRANSPORT_SECTION_INFO_H_
