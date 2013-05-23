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

#ifndef __QUERY_FILTER_H_
#define __QUERY_FILTER_H_

#include <stdint.h>

#include <OsmAndCore.h>
#include <Area.h>

namespace OsmAnd {

    struct OSMAND_CORE_API QueryFilter
    {
        QueryFilter();
    
        const uint32_t* _zoom;
        const AreaI* _bbox31;
    };

} // namespace OsmAnd

#endif // __QUERY_FILTER_H_
