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

#include <QList>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

class SkCanvas;

namespace OsmAnd {

    class RasterizerEnvironment;
    class RasterizerContext;
    namespace Model {
        class MapObject;
    } // namespace Model
    class IQueryController;

    class OSMAND_CORE_API Rasterizer
    {
    private:
        Rasterizer();
        ~Rasterizer();
    public:
        static void prepareContext(
            const RasterizerEnvironment& env,
            RasterizerContext& context,
            const AreaI& area31,
            const ZoomLevel zoom,
            const uint32_t tileSize,
            const MapFoundationType& foundation,
            const QList< std::shared_ptr<const OsmAnd::Model::MapObject> >& objects,
            const PointF& tlOriginOffset = PointF(),
            bool* nothingToRasterize = nullptr,
            IQueryController* controller = nullptr);

        static bool rasterizeMap(
            const RasterizerEnvironment& env,
            const RasterizerContext& context,
            bool fillBackground,
            SkCanvas& canvas,
            IQueryController* controller = nullptr);
    };

} // namespace OsmAnd

#endif // __RASTERIZER_H_
