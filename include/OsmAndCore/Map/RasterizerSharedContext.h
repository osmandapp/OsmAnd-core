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

#ifndef _OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_H_
#define _OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QtGlobal>

#include <OsmAndCore.h>

namespace OsmAnd
{
    class Rasterizer;
    class Rasterizer_P;
    class RasterizerContext;
    class RasterizerContext_P;

    class RasterizerSharedContext_P;
    class OSMAND_CORE_API RasterizerSharedContext
    {
        Q_DISABLE_COPY(RasterizerSharedContext);
    private:
        const std::unique_ptr<RasterizerSharedContext_P> _d;
    protected:
    public:
        RasterizerSharedContext();
        virtual ~RasterizerSharedContext();

    friend class OsmAnd::Rasterizer;
    friend class OsmAnd::Rasterizer_P;
    friend class OsmAnd::RasterizerContext;
    friend class OsmAnd::RasterizerContext_P;
    };

} // namespace OsmAnd

#endif // _OSMAND_CORE_RASTERIZER_SHARED_CONTEXT_H_
