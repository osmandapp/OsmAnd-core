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

bool OsmAnd::Utilities_OpenGL_Common::checkPlaneSegmentIntersection(
    const glm::vec3& planeN, const glm::vec3& planeO,
    const glm::vec3& line0, const glm::vec3& line1,
    float& d0, glm::vec3& outIntersection)
{
    d0 = glm::dot(line0 - planeO, planeN);
    const float d1 = glm::dot(line1 - planeO, planeN);
    
    if (d0 * d1 > 0.0f)
    {
        return false;
    }
    
    const float t = d0 / (d0 - d1);
    outIntersection = line0 + t * (line1 - line0);
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

bool OsmAnd::Utilities_OpenGL_Common::arcIntersectsPlane(const glm::dvec3& planeN, const double planeD,
    const double minAngleX, const double maxAngleX, const double arcZ, const double sqrRadius,
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
    const auto r1 = glm::dvec3((k - planeN.y * y1) / planeN.x, y1, arcZ);
    const auto r2 = glm::dvec3((k - planeN.y * y2) / planeN.x, y2, arcZ);
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

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectsSphere(const glm::dvec3& p0, const glm::dvec3& v,
    const double radius, glm::dvec3& result, bool withBackward /* = false */)
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
    const auto r1p0 = glm::distance(p0, r1);
    const auto r2p0 = glm::distance(p0, r2);
    const bool withR1 = withBackward || glm::dot(r1 - p0, v) > 0.0;
    const bool withR2 = withBackward || glm::dot(r2 - p0, v) > 0.0;
    if (!withR1 && !withR2)
        return false;
    result = glm::normalize((withR1 && withR2 && r2p0 < r1p0) || !withR1 ? r2 : r1);
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectsSphere(const glm::dvec3& p0, const glm::dvec3& v,
    const double radius, double& angleX, double& angleY)
{
    glm::dvec3 result;
    if (!rayIntersectsSphere(p0, v, radius, result, false))
        return false;
    angleX = qAtan2(result.x, result.y);
    angleY = -qAsin(result.z);
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::lineSegmentIntersectsSphere(
    const glm::dvec3& p0, const glm::dvec3& p1, const double radius, double& angleX, double& angleY)
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
    const auto r1p0 = glm::distance(p0, r1);
    const auto r1p1 = glm::distance(p1, r1);
    const auto r2p0 = glm::distance(p0, r2);
    const auto r2p1 = glm::distance(p1, r2);
    const auto vl = glm::length(v);
    const bool withR1 = r1p0 < vl && r1p1 < vl;
    const bool withR2 = r2p0 < vl && r2p1 < vl;
    if (!withR1 && !withR2)
        return false;
    const auto resultPoint = glm::normalize((withR1 && withR2 && r2p0 < r1p0) || !withR1 ? r2 : r1);
    angleX = qAtan2(resultPoint.x, resultPoint.y);
    angleY = -qAsin(resultPoint.z);
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectsCone(
    const glm::dvec3& p0, const glm::dvec3& v, const double nSqrTanLat, double& angleX, double& distance)
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
        const auto r1p0 = glm::distance(p0, r1);
        const auto r2p0 = glm::distance(p0, r2);
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
    distance = glm::length(resultPoint);
    resultPoint /= distance;
    angleX = qAtan2(resultPoint.x, resultPoint.y);
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::planeIntersectsSphere(const glm::dvec3& planeN, const double planeD,
    double& minAngleX, double& maxAngleX, double& minAngleY, double& maxAngleY,
    const glm::dvec3* point1 /* = nullptr */, const glm::dvec3* point2 /* = nullptr */)
{
    const auto pD = fabs(planeD);
    if (pD > 1.0)
        return false;

    bool sectX;
    glm::dvec3 minX, maxX, minY, maxY;
    if (qFuzzyIsNull(planeN.x) && qFuzzyIsNull(planeN.y))
    {
        minAngleX = qMin(minAngleX, -M_PI);
        maxAngleX = qMax(maxAngleX, M_PI);
        const auto mY = -qAsin(qBound(-1.0, planeN.z > 0.0 ? planeD : -planeD, 1.0));
        minAngleY = qMin(minAngleY, mY);
        maxAngleY = qMax(maxAngleY, mY);
        return true;
    }
    else if (qFuzzyIsNull(planeN.z))
    {
        sectX = true;
        const auto pN = planeD < 0.0 ? -planeN : planeN;
        const auto cx = pN.x * pD;
        const auto cy = pN.y * pD;
        const auto r = qSqrt(1.0 - pD * pD);
        const auto rx = pN.y * r;
        const auto ry = -pN.x * r;
        minX = glm::dvec3(cx - rx, cy - ry, 0.0);
        maxX = glm::dvec3(cx + rx, cy + ry, 0.0);
        minY = glm::dvec3(cx, cy, r);
        maxY = glm::dvec3(cx, cy, -r);
    }
    else
    {
        const auto pN = planeD < 0.0 ? -planeN : planeN;
        const auto a = qSqrt(1.0 - pD * pD);
        const auto xyL = qSqrt(1.0 - pN.z * pN.z);
        const auto c = pD * xyL;
        const auto b = a * pN.z;
        sectX = c > fabs(b);
        if (sectX)
        {
            const auto angleXY = qAtan2(pN.x, pN.y);
            const auto deltaAngle = qAtan(a / qSqrt(c * c - b * b));
            const auto a1 = angleXY - deltaAngle;
            const auto a2 = angleXY + deltaAngle;
            const auto absZ = qSqrt(1.0 - c * c);
            const auto z = pN.z > 0 ? absZ : -absZ;
            const auto csy = qCos(-qAsin(qBound(-1.0, z, 1.0)));
            minX = glm::dvec3(csy * qSin(a1), csy * qCos(a1), z);
            maxX = glm::dvec3(csy * qSin(a2), csy * qCos(a2), z);
        }
        const auto cz = pD * pN.z;
        const auto bz = a * xyL;
        const auto xy1 = c + b;
        const auto xy2 = c - b;
        const auto z1 = cz + bz;
        const auto z2 = cz - bz;
        const auto xN = pN.x / xyL;
        const auto yN = pN.y / xyL;
        const auto xYp1 = xN * xy1;
        const auto yYp1 = yN * xy1;
        const auto sxyYp1 = xYp1 * xYp1 + yYp1 * yYp1;
        const auto same = fabs(sxyYp1 + z1 * z1 - 1.0) < fabs(sxyYp1 + z2 * z2 - 1.0);
        const auto zYp1 = same ? z1 : z2;
        const auto xYp2 = xN * xy2;
        const auto yYp2 = yN * xy2;
        const auto zYp2 = same ? z2 : z1;
        const bool change = zYp1 < zYp2;
        minY = change ? glm::dvec3(xYp2, yYp2, zYp2) : glm::dvec3(xYp1, yYp1, zYp1);
        maxY = change ? glm::dvec3(xYp1, yYp1, zYp1) : glm::dvec3(xYp2, yYp2, zYp2);
    }

/*
    else if (isZeroX)
    {
        const auto r = qSqrt(1.0 - planeD * planeD);
        const auto cy = planeD * planeN.y;
        const auto dy = r * planeN.z;
        const auto cz = planeD * planeN.z;
        const auto dz = r * planeN.y;
        const auto z1 = cz + dz;
        const auto z2 = cz - dz;
        const auto yYp1 = cy + dy;
        const auto syYp1 = yYp1 * yYp1;
        const auto same = fabs(syYp1 + z1 * z1 - 1.0) < fabs(syYp1 + z2 * z2 - 1.0);
        const auto zYp1 = same ? z1 : z2;
        const auto yYp2 = cy - dy;
        const auto zYp2 = same ? z2 : z1;
        minY = zYp1 > zYp2 ? glm::dvec3(0.0, yYp1, zYp1) : glm::dvec3(0.0, yYp2, zYp2);
        maxY = zYp1 <= zYp2 ? glm::dvec3(0.0, yYp2, zYp2) : glm::dvec3(0.0, yYp1, zYp1);
    }
    else
    {
        const auto nxs = planeN.x * planeN.x;
        const auto k = nxs + planeN.y * planeN.y;
        const auto nzs = planeN.z * planeN.z;
        const auto a = (nzs + k) * k;
        const auto b = 2.0 * planeD * k * planeN.x;
        const auto c = (planeD * planeD - nzs) * nxs;
        const auto d = b * b - 4.0 * a * c;
        if (d < 0.0)
            return false;
        const auto srd = qSqrt(d);
        const auto a2 = 2.0 * a;
        const auto xYp1 = (b + srd) / a2;
        const auto yYp1 = xYp1 * planeN.y / planeN.x;
        const auto zYp1 = (planeD - xYp1 * k / planeN.x) / planeN.z;
        const auto xYp2 = (b - srd) / a2;
        const auto yYp2 = xYp2 * planeN.y / planeN.x;
        const auto zYp2 = (planeD - xYp2 * k / planeN.x) / planeN.z;
        minY = zYp1 > zYp2 ? glm::dvec3(xYp1, yYp1, zYp1) : glm::dvec3(xYp2, yYp2, zYp2);
        maxY = zYp1 <= zYp2 ? glm::dvec3(xYp2, yYp2, zYp2) : glm::dvec3(xYp1, yYp1, zYp1);
    }
*/

    double minXa, maxXa, minYz, maxYz;
    if (point1 != nullptr && point2 != nullptr)
    {
        const auto fullA = qAtan2(glm::length(glm::cross(*point1, *point2)), glm::dot(*point1, *point2));
        if (sectX)
        {
            const auto angle1 = qAtan2(point1->x, point1->y);
            const auto angle2 = qAtan2(point2->x, point2->y);
            const auto minA = qAtan2(glm::length(glm::cross(*point1, minX)), glm::dot(*point1, minX));
            const auto maxA = qAtan2(glm::length(glm::cross(*point1, maxX)), glm::dot(*point1, maxX));
            minXa = (fullA * minA > 0.0 && fabs(fullA) > fabs(minA)) ? qAtan2(minX.x, minX.y) : qMin(angle1, angle2);
            maxXa = (fullA * maxA > 0.0 && fabs(fullA) > fabs(maxA)) ? qAtan2(maxX.x, maxX.y) : qMax(angle1, angle2);
        }
        else
        {
            minXa = -M_PI;
            maxXa = M_PI;
        }
        const auto minA = qAtan2(glm::length(glm::cross(*point1, minY)), glm::dot(*point1, minY));
        const auto maxA = qAtan2(glm::length(glm::cross(*point1, maxY)), glm::dot(*point1, maxY));
        minYz = (fullA * minA > 0.0 && fabs(fullA) > fabs(minA)) ? minY.z : qMax(point1->z, point2->z);
        maxYz = (fullA * maxA > 0.0 && fabs(fullA) > fabs(maxA)) ? maxY.z : qMin(point1->z, point2->z);
    }
    else
    {
        minXa = sectX ? qAtan2(minX.x, minX.y) : -M_PI;
        maxXa = sectX ? qAtan2(maxX.x, maxX.y) : M_PI;
        minYz = minY.z;
        maxYz = maxY.z;
    }

    minAngleX = qMin(minAngleX, minXa);
    maxAngleX = qMax(maxAngleX, maxXa);
    minAngleY = qMin(minAngleY, -qAsin(qBound(-1.0, minYz, 1.0)));
    maxAngleY = qMax(maxAngleY, -qAsin(qBound(-1.0, maxYz, 1.0)));

    return true;
}
