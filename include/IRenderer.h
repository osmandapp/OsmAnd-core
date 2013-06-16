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
#ifndef __I_RENDERER_H_
#define __I_RENDERER_H_

#include <stdint.h>
#include <memory>

#include <QSet>
#include <QMap>
#include <QMutex>

#include <SkBitmap.h>

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class IMapTileProvider;

    class OSMAND_CORE_API IRenderer
    {
    public:
        enum TextureDepth {
            _16bits,
            _32bits,
        };
    private:
        void tileReadyCallback(const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tile);
    protected:
        IRenderer();

        std::shared_ptr<IMapTileProvider> _tileProvider;

        PointI _windowSize;
        AreaI _viewport;
        float _fieldOfView;
        float _fogDistance;
        float _distanceFromTarget;
        float _azimuth;
        float _elevationAngle;
        PointI _target31;
        uint32_t _zoom;
        bool _viewIsDirty;
        QSet<uint64_t> _visibleTiles;
        PointD _targetInTile;

        TextureDepth _preferredTextureDepth;

        enum {
            TileSide3D = 100,
        };

        virtual void computeMatrices() = 0;
        virtual void refreshVisibleTileset() = 0;
        
        struct OSMAND_CORE_API CachedTile
        {
            virtual ~CachedTile();
        };
        QMutex _tileCacheMutex;
        QMap< uint64_t, std::shared_ptr<CachedTile> > _cachedTiles;//TODO: caching should be done by more complex spherical caching
        virtual void purgeTilesCache();
        void cacheMissingTiles();
        virtual void cacheTile(const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap) = 0;
    public:
        virtual ~IRenderer();

        virtual void setTileProvider(const std::shared_ptr<IMapTileProvider>& tileProvider);
        const std::shared_ptr<IMapTileProvider>& tileProvider;

        const PointI& windowSize;
        const AreaI& viewport;
        const float& fieldOfView;
        const float& fogDistance;
        const float& distanceFromTarget;
        const float& azimuth;
        const float& elevationAngle;
        const PointI& target31;
        const uint32_t& zoom;
        const QSet<uint64_t>& visibleTiles;

        virtual void setPreferredTextureDepth(TextureDepth depth);
        const TextureDepth& preferredTextureDepth;

        virtual int getCachedTilesCount() const;

        virtual bool updateViewport(const PointI& windowSize, const AreaI& viewport, float fieldOfView, float viewDepth);
        virtual bool updateCamera(float distanceFromTarget, float azimuth, float elevationAngle);
        virtual bool updateMap(const PointI& target31, uint32_t zoom);

        const bool& viewIsDirty;
        virtual void refreshView() = 0;

        virtual void performRendering() = 0;
    };

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IRenderer> OSMAND_CORE_CALL createRenderer_OpenGL();
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
}

#endif // __I_RENDERER_H_