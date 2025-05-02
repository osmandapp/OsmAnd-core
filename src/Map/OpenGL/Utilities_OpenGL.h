#ifndef _OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_
#define _OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_

#include "stdlib_common.h"

#include <glm/glm.hpp>

#include "OsmAndCore.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    namespace Utilities_OpenGL_Common
    {
        float calculateCameraDistance(
            const glm::mat4& P,
            const AreaI& viewport,
            const float Ax,
            const float Sx,
            const float k);

        bool rayIntersectPlane(
            const glm::vec3& planeN,
            const glm::vec3& planeO,
            const glm::vec3& rayD,
            const glm::vec3& rayO,
            float& distance);

        bool rayIntersectPlane(
            const glm::dvec3& planeN,
            const glm::dvec3& planeO,
            const glm::dvec3& rayD,
            const glm::dvec3& rayO,
            double& distance);

        bool lineSegmentIntersectPlane(
            const glm::vec3& planeN,
            const glm::vec3& planeO,
            const glm::vec3& line0,
            const glm::vec3& line1,
            glm::vec3& lineX,
            float* distance = nullptr);
    
        bool checkPlaneSegmentIntersection(
            const glm::vec3& planeN,
            const glm::vec3& planeO,
            const glm::vec3& line0,
            const glm::vec3& line1,
            float& d0, glm::vec3& outIntersection);
    }
}

#endif // !defined(_OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_)
