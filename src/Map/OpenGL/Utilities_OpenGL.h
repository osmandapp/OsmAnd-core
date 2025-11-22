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

        bool lineSegmentIntersectsPlane(
            const glm::dvec3& planeN,
            const glm::dvec3& planeO,
            const glm::dvec3& line0,
            const glm::dvec3& line1,
            glm::dvec3& lineX);

        bool arcIntersectsPlane(
            const glm::dvec3& planeN,
            const double planeD,
            const double minAngleX,
            const double maxAngleX,
            const double arcZ,
            const double sqrRadius,
            glm::dvec3& point1,
            glm::dvec3& point2);

        bool rayIntersectsSphere(
            const glm::dvec3& p0,
            const glm::dvec3& v,
            const double radius,
            glm::dvec3& result,
            bool withBackward = false);

        bool rayIntersectsSphere(
            const glm::dvec3& p0,
            const glm::dvec3& v,
            const double radius,
            double& angleX,
            double& angleY);

        bool lineSegmentIntersectsSphere(
            const glm::dvec3& p0,
            const glm::dvec3& p1,
            const double radius,
            double& angleX,
            double& angleY);

        bool rayIntersectsCone(
            const glm::dvec3& p0,
            const glm::dvec3& v,
            const double nSqrTanLat,
            double& angleX,
            double& distance);

        inline glm::dvec4 angleN(const glm::dvec3& v0, const glm::dvec3& v1)
        {
            const auto c = glm::cross(v0, v1);
            const auto result = glm::dvec4(c, qAtan2(glm::length(c), glm::dot(v0, v1)));
            return result;
        }

        inline bool isAngleInside(const glm::dvec4& angleN, const glm::dvec4& limitAngleN)
        {
            bool result = glm::dot(angleN.xyz(), limitAngleN.xyz()) > 0.0 ? angleN.w < limitAngleN.w : false;
            return result;
        }

        bool planeIntersectsSphere(
            const glm::dvec3& planeN,
            const double planeD,
            glm::dvec2& minVectorX,
            glm::dvec2& maxVectorX,
            double& minCoordY,
            double& maxCoordY,
            const glm::dvec3* point1 = nullptr,
            const glm::dvec3* point2 = nullptr);
    }
}

#endif // !defined(_OSMAND_CORE_UTILITIES_OPENGL_COMMON_H_)
