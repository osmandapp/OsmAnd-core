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
#ifndef __MAP_RENDERER_OPENGL_BASE_H_
#define __MAP_RENDERER_OPENGL_BASE_H_

#include <stdint.h>
#include <memory>

#include <QMap>
#include <QMultiMap>

#if defined(WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#endif
#include <GL/gl.h>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <CommonTypes.h>
#include <IMapRenderer.h>

#if !defined(NDEBUG)
#   define GL_CHECK_RESULT validateResult()
#else
#   define GL_CHECK_RESULT
#endif

namespace OsmAnd {

    class OSMAND_CORE_API MapRenderer_BaseOpenGL : public virtual IMapRenderer
    {
    public:
    private:
    protected:
        uint32_t _maxTextureSize;
        uint32_t _atlasSizeOnTexture;
        GLuint _lastUnfinishedAtlas;
        uint32_t _unfinishedAtlasFirstFreeSlot;
        QMultiMap<GLuint, uint32_t> _freeAtlasSlots;
        
        struct OSMAND_CORE_API CachedTile_OpenGL : public IMapRenderer::CachedTile
        {
            CachedTile_OpenGL(MapRenderer_BaseOpenGL* owner, const uint32_t& zoom, const TileId& id, const size_t& usedMemory, GLuint textureId, uint32_t atlasSlotIndex);
            virtual ~CachedTile_OpenGL();

            MapRenderer_BaseOpenGL* const owner;
            const GLuint textureId;
            const uint32_t atlasSlotIndex;
        };
        QMap< GLuint, uint32_t > _texturesRefCounts;
        virtual void purgeTilesCache();

        virtual void releaseTexture(const GLuint& texture) = 0;

        virtual void validateResult() = 0;
        virtual GLuint compileShader(GLenum shaderType, const char* source) = 0;

        MapRenderer_BaseOpenGL();
    public:
        virtual ~MapRenderer_BaseOpenGL();

        virtual int getCachedTilesCount();
    };

}

#endif // __MAP_RENDERER_OPENGL_BASE_H_