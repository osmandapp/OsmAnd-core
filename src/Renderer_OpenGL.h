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
#ifndef __RENDERER_OPENGL_H_
#define __RENDERER_OPENGL_H_

#include <stdint.h>
#include <memory>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <IRenderer.h>

namespace OsmAnd {

    class MapDataCache;

    class OSMAND_CORE_API Renderer_OpenGL : public IRenderer
    {
    private:
        static bool rayIntersectPlane(const glm::vec3& planeN, float planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance);
    protected:
        PointI _glWindowSize;
        AreaI _glViewport;
        glm::mat4 _glProjection;
        glm::mat4 _glModelview;

        virtual void computeMatrices();
        virtual void refreshVisibleTileset();

        struct OSMAND_CORE_API CachedTile_OpenGL : public IRenderer::CachedTile
        {
            virtual ~CachedTile_OpenGL();

            uint32_t textureId;
        };
        virtual void cacheTile(const uint64_t& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap);
    public:
        Renderer_OpenGL();
        virtual ~Renderer_OpenGL();

        virtual void refreshView();

        virtual void performRendering();
    };

}

#endif // __RENDERER_OPENGL_H_