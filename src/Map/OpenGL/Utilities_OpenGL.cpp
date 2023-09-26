#include "Utilities_OpenGL.h"

float OsmAnd::Utilities_OpenGL_Common::calculateCameraDistance(const glm::mat4& P, const AreaI& viewport, const float Ax, const float Sx, const float k)
{
    const float w = viewport.width();
    const float x = viewport.left();

    const float fw = (Sx*k) / (0.5f * w);

    float d = (Ax * P[0][0]) / fw;

    return d;
}

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectPlane(const glm::vec3& planeN, const glm::vec3& planeO, const glm::vec3& rayD, const glm::vec3& rayO, float& distance)
{
    const auto numerator = glm::dot(planeO - rayO, planeN);
    if (qFuzzyIsNull(numerator))
    {
        distance = 0.0f;
        return true;
    }
    const auto denominator = glm::dot(rayD, planeN);
    if (qFuzzyIsNull(denominator))
        return false;

    distance = numerator / denominator;
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectPlane(const glm::dvec3& planeN, const glm::dvec3& planeO, const glm::dvec3& rayD, const glm::dvec3& rayO, double& distance)
{
    const auto numerator = glm::dot(planeO - rayO, planeN);
    if (qFuzzyIsNull(numerator))
    {
        distance = 0.0f;
        return true;
    }
    const auto denominator = glm::dot(rayD, planeN);
    if (qFuzzyIsNull(denominator))
        return false;

    distance = numerator / denominator;
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::lineSegmentIntersectPlane(
    const glm::vec3& planeN, const glm::vec3& planeO,
    const glm::vec3& line0, const glm::vec3& line1,
    glm::vec3& lineX, float* distance /* = nullptr */)
{
    const auto line = line1 - line0;
    const auto length = qMax(glm::length(line), 1e-37f);
    const auto lineD = line / length;
    float d;
    if (!rayIntersectPlane(planeN, planeO, lineD, line0, d))
        return false;

    // If point is not in [line0 .. line1]
    if (d >= 0.0f && d <= length)
    {
        lineX = line0 + d * lineD;
        if (distance)
            *distance = d;
    }
    else
        return false;

    return true;
}
