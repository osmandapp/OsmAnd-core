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

#ifndef __OBF_POI_SECTION_INFO_H_
#define __OBF_POI_SECTION_INFO_H_

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfPoiSectionReader_P;
    class ObfReader_P;

    class OSMAND_CORE_API ObfPoiSectionInfo : public ObfSectionInfo
    {
        Q_DISABLE_COPY(ObfPoiSectionInfo)
    private:
    protected:
        ObfPoiSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        AreaI _area31;
    public:
        virtual ~ObfPoiSectionInfo();

        const AreaI& area31;

        friend class OsmAnd::ObfPoiSectionReader_P;
        friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // __OBF_POI_SECTION_INFO_H_
