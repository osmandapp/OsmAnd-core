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
#ifndef __MAP_RENDERER_OPENGLES2_H_
#define __MAP_RENDERER_OPENGLES2_H_

#include <stdint.h>
#include <memory>
#include <array>

#include <QMap>
#include <QMultiMap>
#include <QSet>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OpenGL_Base/MapRenderer_OpenGL_Base.h>

namespace OsmAnd {

    class OSMAND_CORE_API MapRenderer_OpenGLES2 : public virtual MapRenderer_BaseOpenGL
    {
    public:
    private:
    protected:
        enum {
            BaseBitmapAtlasTilePadding = 2,
            MipmapLodLevelsMax = 4,
        };

        virtual GLenum validateResult();
        virtual GLuint compileShader(GLenum shaderType, const char* source);
        virtual GLuint linkProgram(GLuint shadersCount, GLuint *shaders);

        enum VariableType
        {
            In,
            Uniform
        };
        QMap< GLuint, QMultiMap< VariableType, GLint > > _programVariables;
        void findVariableLocation(GLuint program, GLint& location, const QString& name, const VariableType& type);

        virtual void uploadTileToTexture(TileLayerId layerId, const TileId& tileId, uint32_t zoom, const std::shared_ptr<IMapTileProvider::Tile>& tile, uint64_t& atlasPoolId, void*& textureRef, int& atlasSlotIndex, size_t& usedMemory);
        virtual void releaseTexture(void* textureRef);

        MapRenderer_OpenGLES2();
    public:
        virtual ~MapRenderer_OpenGLES2();

        virtual void initializeRendering();
        virtual void releaseRendering();
    };

}

#endif // __MAP_RENDERER_OPENGLES2_H_