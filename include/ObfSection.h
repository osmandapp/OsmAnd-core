/**
* @file
* @author  Alexey Pelykh <alexey.pelykh@gmail.com>
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

#ifndef __OBF_SECTION_H_
#define __OBF_SECTION_H_

#include <OsmAndCore.h>
#include <string>
#include <stdint.h>

namespace OsmAnd {

    /**
    Base section in OsmAnd binary file
    */
    struct OSMAND_CORE_API ObfSection
    {
        //! Section name
        std::string _name;

        //! Section size in bytes
        uint32_t _length;

        //! Offset in bytes
        uint32_t _offset;
    };

} // namespace OsmAnd

#endif // __OBF_SECTION_H_