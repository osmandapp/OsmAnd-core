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
#ifndef __I_MAP_RENDERER_H_
#define __I_MAP_RENDERER_H_

#include <stdint.h>
#include <memory>
#include <functional>

#include <QSet>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/MapRendererState.h>
#include <OsmAndCore/Map/MapRendererConfiguration.h>

namespace OsmAnd {

    class MapRenderer;
    class OSMAND_CORE_API IMapRenderer
    {
    private:
        MapRendererConfiguration _configuration;
        bool _isRenderingInitialized;

        volatile bool _frameInvalidated;
    protected:
        IMapRenderer();

        MapRendererState _requestedState;
        QList<TileId> _visibleTiles;
    public:
        virtual ~IMapRenderer();

        const MapRendererConfiguration& configuration;
        virtual bool configure(const MapRendererConfiguration& configuration) = 0;

        const bool& isRenderingInitialized;
        virtual bool initializeRendering() = 0;
        virtual bool processRendering() = 0;
        virtual bool renderFrame() = 0;
        virtual bool postprocessRendering() = 0;
        virtual bool releaseRendering() = 0;

        const MapRendererState& state;
        const volatile bool& frameInvalidated;
        const QList<TileId>& visibleTiles;

        virtual void setTileProvider(const MapTileLayerId& layerId, const std::shared_ptr<IMapTileProvider>& tileProvider, bool forcedUpdate = false) = 0;
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false) = 0;
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false) = 0;
        virtual void setFieldOfView(const float& fieldOfView, bool forcedUpdate = false) = 0;
        virtual void setDistanceToFog(const float& fogDistance, bool forcedUpdate = false) = 0;
        virtual void setFogOriginFactor(const float& factor, bool forcedUpdate = false) = 0;
        virtual void setFogHeightOriginFactor(const float& factor, bool forcedUpdate = false) = 0;
        virtual void setFogDensity(const float& fogDensity, bool forcedUpdate = false) = 0;
        virtual void setFogColor(const float& r, const float& g, const float& b, bool forcedUpdate = false) = 0;
        virtual void setSkyColor(const float& r, const float& g, const float& b, bool forcedUpdate = false) = 0;
        virtual void setAzimuth(const float& azimuth, bool forcedUpdate = false) = 0;
        virtual void setElevationAngle(const float& elevationAngle, bool forcedUpdate = false) = 0;
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false) = 0;
        virtual void setZoom(const float& zoom, bool forcedUpdate = false) = 0;
        virtual void setHeightScaleFactor(const float& factor, bool forcedUpdate = false) = 0;

        virtual float getReferenceTileSizeOnScreen() = 0;
        virtual float getScaledTileSizeOnScreen() = 0;

    friend class OsmAnd::MapRenderer;
    };

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createAtlasMapRenderer_OpenGL();
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
#if defined(OSMAND_OPENGLES2_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IMapRenderer> OSMAND_CORE_CALL createAtlasMapRenderer_OpenGLES2();
#endif // OSMAND_OPENGLES2_RENDERER_SUPPORTED
}

#endif // __I_MAP_RENDERER_H_