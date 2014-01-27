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
#ifndef _OSMAND_CORE_MAP_RENDERER_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_STATE_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>
#include <array>

#include <OsmAndCore/QtExtensions.h>
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

    STRONG_ENUM_EX(MapRendererStateChange, uint32_t)
    {
        RasterLayers_Providers = 1 << 0,
        RasterLayers_Opacity = 1 << 1,
        ElevationData_Provider = 1 << 2,
        ElevationData_ScaleFactor = 1 << 3,
        Symbols_Providers = 1 << 4,
        WindowSize = 1 << 5,
        Viewport = 1 << 6,
        FieldOfView = 1 << 7,
        SkyColor = 1 << 8,
        FogParameters = 1 << 9,
        Azimuth = 1 << 10,
        ElevationAngle = 1 << 11,
        Target = 1 << 12,
        Zoom = 1 << 13,
    };

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

#endif // _OSMAND_CORE_MAP_RENDERER_STATE_H_