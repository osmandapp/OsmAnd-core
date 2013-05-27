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
#include <SkPaint.h>
#include <QList>

#include <OsmAndCore.h>
#include <MapObject.h>
#include <IQueryController.h>
#include <RasterizationStyleEvaluator.h>

namespace OsmAnd {

    class RasterizerContext;

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
            RasterizerContext& context,
            const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
            uint32_t zoom,
            QVector< Primitive >& polygons,
            QVector< Primitive >& lines,
            QVector< Primitive >& points,
            IQueryController* controller
            );
        static void filterOutLinesByDensity(RasterizerContext& context, uint32_t zoom, const QVector< Primitive >& in, QVector< Primitive >& out, IQueryController* controller);

        enum PrimitivesType
        {
            Polygons,
            Lines,
            ShadowOnlyLines,
            Points
        };
        static void rasterizePrimitives(RasterizerContext& context, SkCanvas& canvas, uint32_t zoom, const QVector< Primitive >& primitives, PrimitivesType type, IQueryController* controller);
        enum PaintValuesSet : int
        {
            Set_0 = 0,
            Set_1 = 1,
            Set_minus1 = 2,
            Set_minus2 = 3,
            Set_3 = 4,
        };
        static bool updatePaint(RasterizerContext& context, const RasterizationStyleEvaluator& evaluator, PaintValuesSet valueSetSelector, bool isArea);
        static void rasterizePolygon(RasterizerContext& context, SkCanvas& canvas, uint32_t zoom, const Primitive& primitive);
    public:
        virtual ~Rasterizer();

        static bool rasterize(
            RasterizerContext& context,
            SkCanvas& canvas,
            const AreaD& area,
            uint32_t zoom,
            uint32_t tileSidePixelLength,
            const QList< std::shared_ptr<OsmAnd::Model::MapObject> >& objects,
            const PointI& originOffset = PointI(),
            IQueryController* controller = nullptr);
    };

} // namespace OsmAnd

#endif // __RASTERIZER_H_
