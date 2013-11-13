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

#include <cstdint>
#include <memory>

#include <OsmAndCore/QtExtensions.h>
#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

class SkCanvas;

namespace OsmAnd {

    class RasterizerEnvironment;
    class RasterizerContext;
    class RasterizedSymbol;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;

    class Rasterizer_P;
    class OSMAND_CORE_API Rasterizer
    {
        Q_DISABLE_COPY(Rasterizer);
    private:
        const std::unique_ptr<Rasterizer_P> _d;
    protected:
    public:
        Rasterizer(const std::shared_ptr<const RasterizerContext>& context);
        virtual ~Rasterizer();

        const std::shared_ptr<const RasterizerContext> context;

        static void prepareContext(
            RasterizerContext& context,
            const AreaI& area31,
            const ZoomLevel zoom,
            const MapFoundationType foundation,
            const QList< std::shared_ptr<const Model::MapObject> >& objects,
            bool* nothingToRasterize = nullptr,
            const IQueryController* const controller = nullptr);

        void rasterizeMap(
            SkCanvas& canvas,
            const bool fillBackground = true,
            const AreaI* const destinationArea = nullptr,
            const IQueryController* const controller = nullptr);

        //typedef std::function<  > methodToProvideSkCanvasForSize;
        //typedef std::function<  > methodToPublish;
        // a callback-method to provide canvas of specified size?
        //QList< std::shared_ptr<const RasterizedSymbol> >& outSymbols,

        void rasterizeSymbolsWithoutPaths(
            QList< std::shared_ptr<const RasterizedSymbol> >& outSymbols,
            const IQueryController* const controller = nullptr);

        //void rasterizeSymbolsWithPaths(
        //    const IQueryController* const controller = nullptr);
    };

} // namespace OsmAnd

#endif // __RASTERIZER_H_
