#include "Utilities_OpenGL_Base.h"

OSMAND_CORE_API float OSMAND_CORE_CALL OsmAnd::Utilities_BaseOpenGL::calculateCameraDistance( const glm::mat4& P, const AreaI& viewport, const float& Ax, const float& Sx, const float& k )
{
    const float w = viewport.width();
    const float x = viewport.left;

    const float fw = (Sx*k) / (0.5f * w);

    float d = (Ax * P[0][0])/fw;

    return d;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities_BaseOpenGL::rayIntersectPlane( const glm::vec3& planeN, const glm::vec3& planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance )
{
    const auto numerator = glm::dot(planeO - rayO, planeN);
    if(qAbs(numerator) <= std::numeric_limits<float>::epsilon())
    {
        distance = std::numeric_limits<float>::quiet_NaN();
        return true;
    }
    const auto denominator = glm::dot(rayD, planeN);
    if(qAbs(denominator) <= std::numeric_limits<float>::epsilon())
        return false;

    distance = numerator / denominator;
    return true;
}

OSMAND_CORE_API bool OSMAND_CORE_CALL OsmAnd::Utilities_BaseOpenGL::lineSegmentIntersectPlane( const glm::vec3& planeN, const glm::vec3& planeO, const glm::vec3& line0, const glm::vec3& line1, glm::vec3& lineX )
{
    const auto line = line1 - line0;
    const auto lineD = glm::normalize(line);
    float d;
    if(!rayIntersectPlane(planeN, planeO, lineD, line0, d))
        return false;

    // If point is not in [line0 .. line1]
    if(d < 0.0f || d > glm::length(line))
        return false;

    lineX = line0 + d*lineD;
    return true;
}
