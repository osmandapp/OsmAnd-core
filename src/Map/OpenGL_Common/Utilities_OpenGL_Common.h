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
#ifndef _OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_
#define _OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_

#include <cstdint>
#include <memory>

#include <glm/glm.hpp>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd {

    namespace Utilities_OpenGL_Common
    {
        OSMAND_CORE_API float OSMAND_CORE_CALL calculateCameraDistance( const glm::mat4& P, const AreaI& viewport, const float Ax, const float Sx, const float k );
        OSMAND_CORE_API bool OSMAND_CORE_CALL rayIntersectPlane( const glm::vec3& planeN, const glm::vec3& planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance );
        OSMAND_CORE_API bool OSMAND_CORE_CALL lineSegmentIntersectPlane( const glm::vec3& planeN, const glm::vec3& planeO, const glm::vec3& line0, const glm::vec3& line1, glm::vec3& lineX );
    }

}

#endif // _OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_
