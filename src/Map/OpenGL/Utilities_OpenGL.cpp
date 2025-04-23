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
        distance = 0.0;
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

    // If point is in [line0 .. line1]
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

bool OsmAnd::Utilities_OpenGL_Common::lineSegmentIntersectsPlane(
    const glm::dvec3& planeN, const glm::dvec3& planeO,
    const glm::dvec3& line0, const glm::dvec3& line1, glm::dvec3& lineX)
{
    const auto line = line1 - line0;
    const auto length = qMax(glm::length(line), 1e-307);
    const auto lineD = line / length;
    double d;
    if (!rayIntersectPlane(planeN, planeO, lineD, line0, d))
        return false;

    // If point is in [line0 .. line1]
    if (d >= 0.0 && d <= length)
        lineX = line0 + d * lineD;
    else
        return false;

    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::arcIntersectsPlane(const glm::dvec3& planeN, const double& planeD,
    const double& minAngleX, const double& maxAngleX, const double& arcZ, const double& sqrRadius,
    glm::dvec3& point1, glm::dvec3& point2)
{
    const auto k = planeD - planeN.z * arcZ;
    const auto a = planeN.x * planeN.x + planeN.y * planeN.y;
    const auto b = 2.0 * planeN.y * k;
    const auto c = k * k - planeN.x * planeN.x * sqrRadius;
    const auto d = b * b - 4.0 * a * c;
    if (d < 0.0)
        return false;
    const auto srd = qSqrt(d);
    const auto a2 = 2.0 * a;
    const auto y1 = (b + srd) / a2;
    const auto y2 = (b - srd) / a2;
    const auto r1 = glm::dvec3((k + planeN.y * y1) / planeN.x, y1, arcZ);
    const auto r2 = glm::dvec3((k + planeN.y * y2) / planeN.x, y2, arcZ);
    const auto angleR1 = qAtan2(r1.x, r1.y);
    const auto angleR2 = qAtan2(r2.x, r2.y);
    const bool withR1 = angleR1 >= minAngleX && angleR1 <= maxAngleX;
    const bool withR2 = angleR2 >= minAngleX && angleR2 <= maxAngleX;
    if (!withR1 && !withR2)
        return false;
    point1 = withR1 ? r1 : r2;
    point2 = withR2 ? r2 : r1;
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectsSphere(
    const glm::dvec3& p0, const glm::dvec3& v, const double& radius, double& angleX, double& angleY)
{
    const auto a = v.x * v.x + v.y * v.y + v.z * v.z;
    const auto b = 2.0 * (v.x * p0.x + v.y * p0.y + v.z * p0.z);
    const auto c = p0.x * p0.x + p0.y * p0.y + p0.z * p0.z - radius * radius;
    const auto d = b * b - 4.0 * a * c;
    if (d < 0.0)
        return false;
    const auto srd = qSqrt(d);
    const auto a2 = 2.0 * a;
    auto r1 = p0 + v * ((-b + srd) / a2);
    auto r2 = p0 + v * ((-b - srd) / a2);
    const bool r1p0 = glm::distance(p0, r1);
    const bool r2p0 = glm::distance(p0, r2);
    const bool withR1 = glm::dot(r1 - p0, v) > 0.0;
    const bool withR2 = glm::dot(r2 - p0, v) > 0.0;
    if (!withR1 && !withR2)
        return false;
    const auto resultPoint = (withR1 && withR2 && r2p0 < r1p0) || !withR1 ? r2 : r1;
    angleX = qAtan2(resultPoint.x, resultPoint.y);
    angleY = -qAsin(resultPoint.z);
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::lineSegmentIntersectsSphere(
    const glm::dvec3& p0, const glm::dvec3& p1, const double& radius, double& angleX, double& angleY)
{
    const auto v = p1 - p0;
    const auto a = v.x * v.x + v.y * v.y + v.z * v.z;
    const auto b = 2.0 * (v.x * p0.x + v.y * p0.y + v.z * p0.z);
    const auto c = p0.x * p0.x + p0.y * p0.y + p0.z * p0.z - radius * radius;
    const auto d = b * b - 4.0 * a * c;
    if (d < 0.0)
        return false;
    const auto srd = qSqrt(d);
    const auto a2 = 2.0 * a;
    auto r1 = p0 + v * ((-b + srd) / a2);
    auto r2 = p0 + v * ((-b - srd) / a2);
    const bool r1p0 = glm::distance(p0, r1);
    const bool r1p1 = glm::distance(p1, r1);
    const bool r2p0 = glm::distance(p0, r2);
    const bool r2p1 = glm::distance(p1, r2);
    const auto vl = glm::length(v);
    const bool withR1 = r1p0 < vl && r1p1 < vl;
    const bool withR2 = r2p0 < vl && r2p1 < vl;
    if (!withR1 && !withR2)
        return false;
    const auto resultPoint = (withR1 && withR2 && r2p0 < r1p0) || !withR1 ? r2 : r1;
    angleX = qAtan2(resultPoint.x, resultPoint.y);
    angleY = -qAsin(resultPoint.z);
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectsCone(
    const glm::dvec3& p0, const glm::dvec3& v, const double& nSqrTanLat, double& angleX, double& distance)
{
    glm::dvec3 resultPoint;
    double pointDistance;
    if (!qFuzzyIsNull(nSqrTanLat))
    {
        const auto sqrTanLat = qAbs(nSqrTanLat);
        const auto a = v.x * v.x - v.z * v.z / sqrTanLat;
        const auto b = 2.0 * (v.x * p0.x + v.y * p0.y - v.z * p0.z / sqrTanLat);
        const auto c = p0.x * p0.x + p0.y * p0.y - p0.z * p0.z / sqrTanLat;
        const auto d = b * b - 4.0 * a * c;
        if (d < 0.0)
            return false;
        const auto srd = qSqrt(d);
        const auto a2 = 2.0 * a;
        auto r1 = p0 + v * ((-b + srd) / a2);
        auto r2 = p0 + v * ((-b - srd) / a2);
        const bool r1p0 = glm::distance(p0, r1);
        const bool r2p0 = glm::distance(p0, r2);
        const bool withR1 = glm::dot(r1 - p0, v) > 0.0;
        const bool withR2 = glm::dot(r2 - p0, v) > 0.0;
        if (!withR1 && !withR2)
            return false;
        resultPoint = (withR1 && withR2 && r2p0 < r1p0) || !withR1 ? r2 : r1;
        if ((nSqrTanLat > 0.0 && resultPoint.z < 0.0) || (nSqrTanLat < 0.0 && resultPoint.z > 0.0))
            return false;
    }
    else if (rayIntersectPlane(glm::dvec3(0.0, 0.0, 1.0), glm::dvec3(0.0, 0.0, 0.0), v, p0, pointDistance))
        resultPoint = p0 + v * pointDistance;
    else
        return false;
    angleX = qAtan2(resultPoint.x, resultPoint.y);
    distance = glm::length(resultPoint);
    return true;
}
