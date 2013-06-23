#include "Utilities_OpenGL_Base.h"

OSMAND_CORE_API float OSMAND_CORE_CALL OsmAnd::Utilities_BaseOpenGL::calculateCameraDistance( const glm::mat4& P, const AreaI& viewport, const float& Ax, const float& Sx, const float& k )
{
    const float w = viewport.width();
    const float x = viewport.left;

    const float fw = (Sx*k) / (0.5f * viewport.width());

    float d = (P[3][0] + Ax*P[0][0] - fw*(P[3][3] + Ax*P[0][3]))/(P[2][0] - fw*P[2][3]);

    return d;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities_BaseOpenGL::rayIntersectPlane( const glm::vec3& planeN, float planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance )
{
    auto alpha = glm::dot(planeN, rayD);

    if(!qFuzzyCompare(alpha, 0.0f))
    {
        distance = (-glm::dot(planeN, rayO) + planeO) / alpha;

        return (distance >= 0.0f);
    }

    return false;
}

