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
#ifndef __MAP_RENDERER_STATE_H_
#define __MAP_RENDERER_STATE_H_

#include <cstdint>
#include <memory>
#include <functional>
#include <array>

#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>

namespace OsmAnd {

    class IMapBitmapTileProvider;
    class IMapElevationDataProvider;
    class IMapSymbolProvider;
    class IMapRenderer;
    class MapRenderer;

    class OSMAND_CORE_API MapRendererState
    {
    private:
    protected:
        MapRendererState();
    public:
        virtual ~MapRendererState();

        std::array< std::shared_ptr<IMapBitmapTileProvider>, RasterMapLayersCount > rasterLayerProviders;
        std::array< float, RasterMapLayersCount > rasterLayerOpacity;
        std::shared_ptr<IMapElevationDataProvider> elevationDataProvider;
        float elevationDataScaleFactor;
        QSet< std::shared_ptr<IMapSymbolProvider> > symbolProviders;
        PointI windowSize;
        AreaI viewport;
        float fieldOfView;
        FColorRGB skyColor;
        FColorRGB fogColor;
        float fogDistance;
        float fogOriginFactor;
        float fogHeightOriginFactor;
        float fogDensity;
        float azimuth;
        float elevationAngle;
        PointI target31;
        float requestedZoom;
        ZoomLevel zoomBase;
        float zoomFraction;

    friend class OsmAnd::IMapRenderer;
    friend class OsmAnd::MapRenderer;
    };

}

#endif // __MAP_RENDERER_STATE_H_