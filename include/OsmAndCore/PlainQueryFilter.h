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

#ifndef __PLAIN_QUERY_FILTER_H_
#define __PLAIN_QUERY_FILTER_H_

#include <cstdint>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IQueryFilter.h>

namespace OsmAnd {

    class OSMAND_CORE_API PlainQueryFilter : public IQueryFilter
    {
    private:
    protected:
        bool _isZoomFiltered;
        uint32_t _zoom;
        bool _isAreaFiltered;
        AreaI _area;
    public:
        PlainQueryFilter(const uint32_t* zoom, const AreaI* area);
        virtual ~PlainQueryFilter();

        virtual bool acceptsZoom(uint32_t zoom);
        virtual bool acceptsArea(const AreaI& area);
        virtual bool acceptsPoint(const PointI& point);
    };

} // namespace OsmAnd

#endif // __PLAIN_QUERY_FILTER_H_
