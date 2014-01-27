/**
* @file
*
* @section LICENSE
*
* OsmAnd - Android navigation software based on OSM maps.
* Copyright (C) 2010-2014  OsmAnd Authors listed in AUTHORS file
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

#ifndef _OSMAND_CORE_OBF_SECTION_INFO_H_
#define _OSMAND_CORE_OBF_SECTION_INFO_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>
#include <QAtomicInt>

#include <OsmAndCore.h>

namespace OsmAnd {

    class ObfInfo;
    class ObfReader;
    class ObfReader_P;

    class OSMAND_CORE_API ObfSectionInfo
    {
    private:
        static QAtomicInt _nextRuntimeGeneratedId;
    protected:
        ObfSectionInfo(const std::weak_ptr<ObfInfo>& owner);

        QString _name;
        uint32_t _length;
        uint32_t _offset;
    public:
        virtual ~ObfSectionInfo();

        const QString& name;
        const uint32_t &length;
        const uint32_t &offset;

        const int runtimeGeneratedId;
        
        std::weak_ptr<ObfInfo> owner;

    friend class OsmAnd::ObfReader;
    friend class OsmAnd::ObfReader_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_OBF_SECTION_INFO_H_
