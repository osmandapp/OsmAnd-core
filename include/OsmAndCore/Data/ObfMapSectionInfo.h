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

#ifndef __OBF_MAP_SECTION_INFO_H_
#define __OBF_MAP_SECTION_INFO_H_

#include <stdint.h>
#include <memory>

#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfSectionInfo.h>

namespace OsmAnd {

    class ObfMapSectionReader_P;
    class ObfReader_P;

    class OSMAND_CORE_API ObfMapSectionLevel
    {
    private:
    protected:
        ObfMapSectionLevel();

        uint32_t _offset;
        uint32_t _length;
        ZoomLevel _minZoom;
        ZoomLevel _maxZoom;
        AreaI _area31;

        uint32_t _boxesOffset;
    public:
        virtual ~ObfMapSectionLevel();

        const uint32_t& offset;
        const uint32_t& length;
        
        const ZoomLevel& minZoom;
        const ZoomLevel& maxZoom;
        const AreaI& area31;

    friend class OsmAnd::ObfMapSectionReader_P;
    };

    class ObfMapSectionInfo_P;
    class OSMAND_CORE_API ObfMapSectionInfo : public ObfSectionInfo
    {
    private:
        const std::unique_ptr<ObfMapSectionInfo_P> _d;
    protected:
        ObfMapSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        bool _isBasemap;
        QList< std::shared_ptr<const ObfMapSectionLevel> > _levels;
    public:
        virtual ~ObfMapSectionInfo();

        const bool& isBasemap;
        const QList< std::shared_ptr<const ObfMapSectionLevel> >& levels;

    friend class OsmAnd::ObfMapSectionReader_P;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // __OBF_MAP_SECTION_INFO_H_
