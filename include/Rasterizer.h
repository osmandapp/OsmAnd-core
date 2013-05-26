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

#ifndef __RASTERIZER_H_
#define __RASTERIZER_H_

#include <stdint.h>
#include <memory>

#include <SkCanvas.h>
#include <QList>

#include <OsmAndCore.h>
#include <MapObject.h>
#include <IQueryController.h>

namespace OsmAnd {

    class RasterizationStyleContext;

    class OSMAND_CORE_API Rasterizer
    {
    private:
        Rasterizer();

        enum {
            MaxV = 75,
        };
    protected:
        enum PrimitiveType : uint32_t
        {
            Point = 1,
            Line = 2,
            Polygon = 3,
        };

        struct Primitive
        {
            std::shared_ptr<OsmAnd::Model::MapObject> mapObject;
            double zOrder;
            uint32_t typeIndex;
            PrimitiveType objectType;
        };

        static void obtainPrimitives(
            const RasterizationStyleContext& styleContext,
            const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
            uint32_t zoom,
            QList< Primitive >& polygons,
            QList< Primitive >& lines,
            QList< Primitive >& points
            );
    public:
        virtual ~Rasterizer();

        static bool rasterize(
            SkCanvas& canvas,
            const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
            uint32_t zoom,
            const RasterizationStyleContext& styleContext,
            IQueryController* controller = nullptr);
    };

} // namespace OsmAnd

#endif // __RASTERIZER_H_
