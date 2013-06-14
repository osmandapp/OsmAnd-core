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

#include <OsmAndCore.h>
#include <CommonTypes.h>

namespace OsmAnd {

    class MapDataCache;

    class OSMAND_CORE_API IRenderer
    {
    private:
    protected:
        IRenderer();

        std::shared_ptr<MapDataCache> _source;

        PointI _windowSize;
        AreaI _viewport;
        float _fieldOfView;
        float _fogDistance;
        float _distanceFromTarget;
        float _azimuth;
        float _elevationAngle;
        PointI _target31;
        uint32_t _zoom;
        bool _matricesAreDirty;
        bool _tilesetIsDirty;
        QSet<uint64_t> _visibleTiles;

        enum {
            TileSide3D = 100,
        };
    public:
        virtual ~IRenderer();

        virtual void setSource(const std::shared_ptr<MapDataCache>& source);
        const std::shared_ptr<MapDataCache>& source;

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

        virtual bool updateViewport(const PointI& windowSize, const AreaI& viewport, float fieldOfView, float viewDepth);
        virtual bool updateCamera(float distanceFromTarget, float azimuth, float elevationAngle);
        virtual bool updateMap(const PointI& target31, uint32_t zoom);

        const bool& matricesAreDirty;
        virtual bool computeMatrices() = 0;

        const bool& tilesetIsDirty;
        virtual void refreshVisibleTileset() = 0;

        virtual void performRendering() const = 0;
    };

#if defined(OSMAND_OPENGL_RENDERER_SUPPORTED)
    OSMAND_CORE_API std::shared_ptr<OsmAnd::IRenderer> OSMAND_CORE_CALL createRenderer_OpenGL();
#endif // OSMAND_OPENGL_RENDERER_SUPPORTED
}

#endif // __I_RENDERER_H_