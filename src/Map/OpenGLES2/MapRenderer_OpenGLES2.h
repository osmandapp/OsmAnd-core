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
        virtual GLenum validateResult();

        MapRenderer_OpenGLES2();
    public:
        virtual ~MapRenderer_OpenGLES2();

        virtual void initializeRendering();
        virtual void releaseRendering();
    };

}

#endif // __MAP_RENDERER_OPENGLES2_H_