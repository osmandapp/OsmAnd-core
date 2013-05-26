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

#ifndef __RASTERIZATION_STYLE_CONTEXT_H_
#define __RASTERIZATION_STYLE_CONTEXT_H_

#include <stdint.h>
#include <memory>

#include <OsmAndCore.h>

namespace OsmAnd {

    class RasterizationStyle;

    class OSMAND_CORE_API RasterizationStyleContext
    {
    private:
    protected:
    public:
        RasterizationStyleContext(const std::shared_ptr<RasterizationStyle>& style);
        virtual ~RasterizationStyleContext();

        const std::shared_ptr<RasterizationStyle> style;
    };


} // namespace OsmAnd

#endif // __RASTERIZATION_STYLE_CONTEXT_H_
