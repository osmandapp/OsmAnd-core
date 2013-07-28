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
#ifndef __MAP_RENDERER_H_
#define __MAP_RENDERER_H_

#include <stdint.h>
#include <memory>
#include <functional>
#include <array>

#include <QReadWriteLock>
#include <QWaitCondition>
#include <QSet>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <MapTypes.h>
#include <Concurrent.h>
#include <IMapRenderer.h>
#include <IMapTileProvider.h>
#include <MapRendererTileLayer.h>
#include <TileZoomCache.h>

namespace OsmAnd {

    class RenderAPI;
    class OSMAND_CORE_API MapRenderer : public IMapRenderer
    {
    private:
        QReadWriteLock _stateLock;
        MapRendererState _currentState;
        volatile bool _currentStateInvalidated;

        std::array< std::unique_ptr<MapRendererTileLayer>, MapTileLayerIdsCount > _layers;

        std::unique_ptr<RenderAPI> _renderAPI;
    protected:
        MapRenderer();

        virtual bool preInitializeRendering();
        virtual bool doInitializeRendering();
        virtual bool postInitializeRendering();

        virtual bool preProcessRendering();
        virtual bool doProcessRendering();
        virtual bool postProcessRendering();

        virtual bool preRenderFrame();
        virtual bool doRenderFrame() = 0;
        virtual bool postRenderFrame();

        virtual bool prePostprocessRendering();
        virtual bool doPostprocessRendering();
        virtual bool postPostprocessRendering();

        virtual bool preReleaseRendering();
        virtual bool doReleaseRendering();
        virtual bool postReleaseRendering();

        const MapRendererState& currentState;
        void invalidateCurrentState();
        virtual bool updateCurrentState();

        TileId _targetTileId;
        PointF _targetInTileOffsetN;

        QReadWriteLock _invalidatedLayersLock;
        volatile uint32_t _invalidatedLayers;
        void invalidateLayer(const MapTileLayerId& layerId);
        virtual void validateLayer(const MapTileLayerId& layerId);

        void invalidateFrame();

        QSet<TileId> _uniqueTiles;
        void requestMissingTiles();
        void processRequestedTile(const MapTileLayerId& layerId, const TileId& tileId, const ZoomLevel& zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, bool success);

        Qt::HANDLE _renderThreadId;
        Qt::HANDLE _workerThreadId;

        std::unique_ptr<Concurrent::Thread> _backgroundWorker;
        QWaitCondition _backgroundWorkerWakeup;
        void backgroundWorkerProcedure();

        const std::array< std::unique_ptr<MapRendererTileLayer>, MapTileLayerIdsCount >& layers;

        const std::unique_ptr<RenderAPI>& renderAPI;
        virtual RenderAPI* allocateRenderAPI() = 0;
    public:
        virtual ~MapRenderer();

        virtual bool configure(const MapRendererConfiguration& configuration);

        virtual bool initializeRendering();
        virtual bool processRendering();
        virtual bool renderFrame();
        virtual bool postprocessRendering();
        virtual bool releaseRendering();

        virtual void setTileProvider(const MapTileLayerId& layerId, const std::shared_ptr<IMapTileProvider>& tileProvider, bool forcedUpdate = false);
        virtual void setWindowSize(const PointI& windowSize, bool forcedUpdate = false);
        virtual void setViewport(const AreaI& viewport, bool forcedUpdate = false);
        virtual void setFieldOfView(const float& fieldOfView, bool forcedUpdate = false);
        virtual void setDistanceToFog(const float& fogDistance, bool forcedUpdate = false);
        virtual void setFogOriginFactor(const float& factor, bool forcedUpdate = false);
        virtual void setFogHeightOriginFactor(const float& factor, bool forcedUpdate = false);
        virtual void setFogDensity(const float& fogDensity, bool forcedUpdate = false);
        virtual void setFogColor(const float& r, const float& g, const float& b, bool forcedUpdate = false);
        virtual void setSkyColor(const float& r, const float& g, const float& b, bool forcedUpdate = false);
        virtual void setAzimuth(const float& azimuth, bool forcedUpdate = false);
        virtual void setElevationAngle(const float& elevationAngle, bool forcedUpdate = false);
        virtual void setTarget(const PointI& target31, bool forcedUpdate = false);
        virtual void setZoom(const float& zoom, bool forcedUpdate = false);
        virtual void setHeightScaleFactor(const float& factor, bool forcedUpdate = false);
    };

}

#endif // __MAP_RENDERER_H_