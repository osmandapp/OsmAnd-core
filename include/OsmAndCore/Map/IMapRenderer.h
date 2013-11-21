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
#ifndef _OSMAND_CORE_I_MAP_RENDERER_H_
#define _OSMAND_CORE_I_MAP_RENDERER_H_

#include <cstdint>
#include <memory>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/MapRendererTypes.h>
#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/MapRendererConfiguration.h>
#include <OsmAndCore/Map/MapRendererSetupOptions.h>

namespace OsmAnd {

    class MapRenderer;
    class IMapBitmapTileProvider;
    class IMapElevationDataProvider;
    class IMapSymbolProvider;

    class OSMAND_CORE_API IMapRenderer
    {
    private:
        bool _isRenderingInitialized;

        MapRendererSetupOptions _setupOptions;

        MapRendererConfiguration _requestedConfiguration;

        volatile bool _frameInvalidated;
    protected:
        IMapRenderer();

        MapRendererState _requestedState;
    public:
        virtual ~IMapRenderer();

        const MapRendererSetupOptions& setupOptions;
        virtual bool setup(const MapRendererSetupOptions& setupOptions) = 0;

        const MapRendererConfiguration& configuration;
        virtual void setConfiguration(const MapRendererConfiguration& configuration, bool forcedUpdate = false) = 0;

        const bool& isRenderingInitialized;
        virtual bool initializeRendering() = 0;
        virtual bool prepareFrame() = 0;
        virtual bool renderFrame() = 0;
        virtual bool processRendering() = 0;
        virtual bool releaseRendering() = 0;

        const MapRendererState& state;
        const volatile bool& frameInvalidated;
        virtual unsigned int getVisibleTilesCount() const = 0;

        virtual void setRasterLayerProvider(const RasterMapLayerId layerId, const std::shared_ptr<IMapBitmapTileProvider>& tileProvider, bool forcedUpdate = false) = 0;
        virtual void resetRasterLayerProvider(const RasterMapLayerId layerId, bool forcedUpdate = false) = 0;
        virtual void setRasterLayerOpacity(const RasterMapLayerId layerId, const float opacity, bool forcedUpdate = false) = 0;
        virtual void setElevationDataProvider(const std::shared_ptr<IMapElevationDataProvider>& tileProvider, bool forcedUpdate = false) = 0;
        virtual void resetElevationDataProvider(bool forcedUpdate = false) = 0;
        virtual void setElevationDataScaleFactor(const float factor, bool forcedUpdate = false) = 0;
        virtual void addSymbolProvider(const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate = false) = 0;
        virtual void removeSymbolProvider(const std::shared_ptr<IMapSymbolProvider>& provider, bool forcedUpdate = false) = 0;
        virtual void removeAllSymbolProviders(bool forcedUpdate = false) = 0;
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false) = 0;
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false) = 0;
        virtual void setFieldOfView(const float fieldOfView, bool forcedUpdate = false) = 0;
        virtual void setDistanceToFog(const float fogDistance, bool forcedUpdate = false) = 0;
        virtual void setFogOriginFactor(const float factor, bool forcedUpdate = false) = 0;
        virtual void setFogHeightOriginFactor(const float factor, bool forcedUpdate = false) = 0;
        virtual void setFogDensity(const float fogDensity, bool forcedUpdate = false) = 0;
        virtual void setFogColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual void setSkyColor(const FColorRGB& color, bool forcedUpdate = false) = 0;
        virtual void setAzimuth(const float azimuth, bool forcedUpdate = false) = 0;
        virtual void setElevationAngle(const float elevationAngle, bool forcedUpdate = false) = 0;
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false) = 0;
        virtual void setZoom(const float zoom, bool forcedUpdate = false) = 0;

        virtual float getReferenceTileSizeOnScreen() = 0;
        virtual float getScaledTileSizeOnScreen() = 0;
        //NOTE: screen points origin from top-left
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI& location31) = 0;
        virtual bool getLocationFromScreenPoint(const PointI& screenPoint, PointI64& location) = 0;

    friend class OsmAnd::MapRenderer;
    };

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createAtlasMapRenderer_OpenGL();
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
#if defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createAtlasMapRenderer_OpenGLES2();
#endif // OSMAND_OPENGLES2_RENDERER_SUPPORTED
}

#endif // _OSMAND_CORE_I_MAP_RENDERER_H_
