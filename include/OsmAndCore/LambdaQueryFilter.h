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

#ifndef _OSMAND_CORE_LAMBDA_QUERY_FILTER_H_
#define _OSMAND_CORE_LAMBDA_QUERY_FILTER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IQueryFilter.h>

namespace OsmAnd {

    class OSMAND_CORE_API LambdaQueryFilter : public IQueryFilter
    {
    public:
        typedef std::function<bool (uint32_t zoom)> ZoomFunctionSignature;
        typedef std::function<bool (const AreaI& area)> AreaFunctionSignature;
        typedef std::function<bool (const PointI& point)> PointFunctionSignature;
    private:
    protected:
        ZoomFunctionSignature _zoomFunction;
        AreaFunctionSignature _areaFunction;
        PointFunctionSignature _pointFunction;
    public:
        LambdaQueryFilter(ZoomFunctionSignature zoomFunction, AreaFunctionSignature areaFunction, PointFunctionSignature pointFunction);
        virtual ~LambdaQueryFilter();

        virtual bool acceptsZoom(uint32_t zoom);
        virtual bool acceptsArea(const AreaI& area);
        virtual bool acceptsPoint(const PointI& point);
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_LAMBDA_QUERY_FILTER_H_
