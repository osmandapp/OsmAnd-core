#ifndef _OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_
#define _OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_

#include <OsmAndCore/stdlib_common.h>

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

#endif // !defined(_OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_)
