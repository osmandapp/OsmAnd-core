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

#include <QMap>
#include <QQueue>
#include <QSet>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <BaseAtlasMapRenderer.h>

namespace OsmAnd {

    class MapDataCache;

    class OSMAND_CORE_API AtlasMapRenderer_OpenGL : public BaseAtlasMapRenderer
    {
    private:
        static bool rayIntersectPlane(const glm::vec3& planeN, float planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance);
        static void validateResult();
        static float calculateCameraDistance(const glm::mat4& P, const AreaI& viewport, float Ax, float Sx, float k);
    protected:
        glm::mat4 _glProjection;
        glm::mat4 _glModelview;
        float _distanceFromCameraToTarget;
        uint32_t _glMaxTextureDimension;
        uint32_t _lastUnfinishedAtlas;
        uint32_t _unfinishedAtlasFirstFreeSlot;
        QQueue<uint64_t> _freeAtlasSlots;
        Qt::HANDLE _glRenderThreadId;

        virtual void computeMatrices();
        virtual void refreshVisibleTileset();

        struct OSMAND_CORE_API CachedTile_OpenGL : public IMapRenderer::CachedTile
        {
            CachedTile_OpenGL(AtlasMapRenderer_OpenGL* owner, const uint32_t& zoom, const TileId& id, const size_t& usedMemory, uint32_t textureId, uint32_t atlasSlotIndex);
            virtual ~CachedTile_OpenGL();

            AtlasMapRenderer_OpenGL* const owner;
            const uint32_t textureId;
            const uint32_t atlasSlotIndex;
        };
        virtual void cacheTile(const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap);
        void uploadTileToTexture(const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap);
        QMap< uint32_t, uint32_t > _glTexturesRefCounts;

        virtual void updateConfiguration();
    public:
        AtlasMapRenderer_OpenGL();
        virtual ~AtlasMapRenderer_OpenGL();

        virtual int getCachedTilesCount();

        virtual void initializeRendering();
        virtual void performRendering();
        virtual void releaseRendering();
    };

}

#endif // __RENDERER_OPENGL_H_