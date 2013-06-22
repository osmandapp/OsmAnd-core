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
#ifndef __BASE_ATLAS_MAP_RENDERER_OPENGL_H_
#define __BASE_ATLAS_MAP_RENDERER_OPENGL_H_

#include <stdint.h>
#include <memory>

#include <QQueue>

#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <BaseAtlasMapRenderer.h>

namespace OsmAnd {

    class OSMAND_CORE_API BaseAtlasMapRenderer_OpenGL : public BaseAtlasMapRenderer
    {
    public:
    private:
    protected:
        BaseAtlasMapRenderer_OpenGL();

        glm::mat4 _mProjection;
        glm::mat4 _mView;
        float _distanceFromCameraToTarget;
        uint32_t _maxTextureSize;
        uint32_t _atlasSizeOnTexture;
        GLuint _lastUnfinishedAtlas;
        uint32_t _unfinishedAtlasFirstFreeSlot;
        QMultiMap<GLuint, uint32_t> _freeAtlasSlots;
        Qt::HANDLE _renderThreadId;

        static float calculateCameraDistance(const glm::mat4& P, const AreaI& viewport, const float& Ax, const float& Sx, const float& k);
        static bool rayIntersectPlane(const glm::vec3& planeN, float planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance);

        void computeMatrices();
        void computeVisibleTileset();

        struct OSMAND_CORE_API CachedTile_OpenGL : public IMapRenderer::CachedTile
        {
            CachedTile_OpenGL(BaseAtlasMapRenderer_OpenGL* owner, const uint32_t& zoom, const TileId& id, const size_t& usedMemory, GLuint textureId, uint32_t atlasSlotIndex);
            virtual ~CachedTile_OpenGL();

            BaseAtlasMapRenderer_OpenGL* const owner;
            const GLuint textureId;
            const uint32_t atlasSlotIndex;
        };
        virtual void cacheTile(const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap);
        virtual void uploadTileToTexture(const TileId& tileId, uint32_t zoom, const std::shared_ptr<SkBitmap>& tileBitmap) = 0;
        virtual void releaseTexture(const GLuint& texture) = 0;
        QMap< GLuint, uint32_t > _texturesRefCounts;

#pragma pack(push)
#pragma pack(1)
        struct Vertex 
        {
            GLfloat position[3];
            GLfloat uv[2];
        };
#pragma pack(pop)

        virtual void createTilePatch();
        virtual void allocateTilePatch(Vertex* vertices, size_t verticesCount, GLushort* indices, size_t indicesCount) = 0;
        virtual void releaseTilePatch() = 0;
        
        virtual void updateConfiguration();

        virtual void purgeTilesCache();
    public:
        virtual ~BaseAtlasMapRenderer_OpenGL();

        virtual void initializeRendering();
        virtual void performRendering();
        virtual void releaseRendering();

        virtual int getCachedTilesCount();
    };

}

#endif // __BASE_ATLAS_MAP_RENDERER_OPENGL_H_