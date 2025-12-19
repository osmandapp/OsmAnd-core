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
    const auto r1p0 = glm::dot(r1 - p0, v);
    const auto r2p0 = glm::dot(r2 - p0, v);
    const bool inFrontR1 = r1p0 > 0.0;
    const bool inFrontR2 = r2p0 > 0.0;
    const bool withR1 = withBackward || inFrontR1;
    const bool withR2 = withBackward || inFrontR2;
    if (!withR1 && !withR2)
        return false;
    const bool takeR2 = withBackward && (!inFrontR1 || !inFrontR2)
        ? (!inFrontR1 && !inFrontR2 ? fabs(r1p0) < fabs(r2p0) : !inFrontR2) : fabs(r2p0) < fabs(r1p0);
    result = (withR1 && withR2 && takeR2) || !withR1 ? r2 : r1;
    return true;
}

bool OsmAnd::Utilities_OpenGL_Common::rayIntersectsSphere(const glm::dvec3& p0, const glm::dvec3& v,
    const double radius, double& angleX, double& angleY)
{
    glm::dvec3 result;
    if (!rayIntersectsSphere(p0, v, radius, result))
        return false;
    if (radius > 1.0)
        result = glm::normalize(result);
    angleX = qAtan2(result.x, result.y);
    angleY = -qAsin(qBound(-1.0, result.z, 1.0));
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
    glm::dvec2& minVectorX, glm::dvec2& maxVectorX, double& minCoordY, double& maxCoordY,
    const glm::dvec3* point1 /* = nullptr */, const glm::dvec3* point2 /* = nullptr */)
{
    const auto pD = fabs(planeD);
    if (pD > 1.0)
        return false;

    const auto fullCircle = 2.0 * M_PI;

    double r;
    bool sectX;
    glm::dvec3 minX, maxX, minY, maxY;
    if (qFuzzyIsNull(planeN.x) && qFuzzyIsNull(planeN.y))
    {
        minVectorX = maxVectorX = glm::dvec2(0.0, -1.0);
        const auto mY = planeN.z > 0.0 ? planeD : -planeD;
        minCoordY = qMin(minCoordY, mY);
        maxCoordY = qMax(maxCoordY, mY);
        return true;
    }
    else if (qFuzzyIsNull(planeN.z))
    {
        sectX = true;
        const auto pN = planeD < 0.0 ? -planeN : planeN;
        const auto cx = pN.x * pD;
        const auto cy = pN.y * pD;
        r = qSqrt(1.0 - pD * pD);
        const auto rx = pN.y * r;
        const auto ry = -pN.x * r;
        minX = glm::dvec3(cx - rx, cy - ry, 0.0);
        maxX = glm::dvec3(cx + rx, cy + ry, 0.0);
        minY = glm::dvec3(cx, cy, -r);
        maxY = glm::dvec3(cx, cy, r);
    }
    else
    {
        const auto pN = planeD < 0.0 ? -planeN : planeN;
        const auto spD = pD * pD;
        const auto sr = 1.0 - spD;
        r = qSqrt(sr);
        const auto xyL = qSqrt(1.0 - pN.z * pN.z);
        const auto c = pD * xyL;
        const auto b = r * pN.z;
        sectX = c > fabs(b);
        if (sectX)
        {
            const auto sb = b * b;
            const auto sc = c * c;
            const auto angleXY = qAtan2(pN.x, pN.y);
            const auto scmb = qSqrt(sc - sb);
            const auto deltaAngle = qAtan(r / scmb);
            const auto a1 = angleXY - deltaAngle;
            const auto a2 = angleXY + deltaAngle;
            const auto absZ = qSqrt(1.0 - scmb * qSqrt(sr - sb + sc) / c);
            const auto z = pN.z > 0 ? absZ : -absZ;
            const auto csy = qCos(-qAsin(qBound(-1.0, z, 1.0)));
            minX = glm::dvec3(csy * qSin(a1), csy * qCos(a1), z);
            maxX = glm::dvec3(csy * qSin(a2), csy * qCos(a2), z);
        }
        const auto cz = pD * pN.z;
        const auto bz = r * xyL;
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
        minY = change ? glm::dvec3(xYp1, yYp1, zYp1) : glm::dvec3(xYp2, yYp2, zYp2);
        maxY = change ? glm::dvec3(xYp2, yYp2, zYp2) : glm::dvec3(xYp1, yYp1, zYp1);
    }

    glm::dvec2 minXv, maxXv;
    double minYz, maxYz;
    if (point1 != nullptr && point2 != nullptr)
    {
        const auto center = planeN * planeD;
        const auto v1 = *point1 - center;
        const auto v2 = *point2 - center;
        const auto fullA = angleN(v1, v2);
        bool minInside = false;
        bool maxInside = false;
        if (sectX)
        {
            const auto cminX = minX - center;
            const auto cmaxX = maxX - center;
            const auto minA = angleN(v1, cminX);
            const auto maxA = angleN(v1, cmaxX);
            minInside = isAngleInside(minA, fullA);
            maxInside = isAngleInside(maxA, fullA);
            minXv = minX.xy();
            maxXv = maxX.xy();
        }
        glm::dvec2 minXp, maxXp;
        if (!minInside || !maxInside)
        {
            const auto angle1 = qAtan2(point1->x, point1->y);
            const auto angle2 = qAtan2(point2->x, point2->y);
            const auto minAngle = qMin(angle1, angle2);
            const auto maxAngle = qMax(angle1, angle2);
            minXp = angle2 > angle1 ? point1->xy() : point2->xy();
            maxXp = angle2 > angle1 ? point2->xy() : point1->xy();
            if (!sectX)
            {
                const auto vM = glm::normalize(v1 + v2) * r + center;
                const auto angleM = qAtan2(vM.x, vM.y);
                if (angleM < minAngle || angleM > maxAngle)
                {
                    auto minXpt = minXp;
                    minXp = maxXp;
                    maxXp = minXpt;
                }
            }
            else if (maxAngle - minAngle > M_PI)
            {
                auto minXpt = minXp;
                minXp = maxXp;
                maxXp = minXpt;
            }
        }
        if (!minInside)
            minXv = minXp;
        if (!maxInside)
            maxXv = maxXp;
        const auto cminY = minY - center;
        const auto cmaxY = maxY - center;
        const auto minA = angleN(v1, cminY);
        const auto maxA = angleN(v1, cmaxY);
        minYz = isAngleInside(minA, fullA) ? minY.z : qMin(point1->z, point2->z);
        maxYz = isAngleInside(maxA, fullA) ? maxY.z : qMax(point1->z, point2->z);
    }
    else
    {
        minXv = sectX ? minX.xy() : glm::dvec2(0.0, -1.0);
        maxXv = sectX ? maxX.xy() : glm::dvec2(0.0, -1.0);
        minYz = minY.z;
        maxYz = maxY.z;
    }

    minXv = glm::normalize(minXv);
    maxXv = glm::normalize(maxXv);
    if (minXv == maxXv && minXv.x == 0.0 && minXv.y == -1.0)
    {
        minVectorX = maxVectorX = minXv;
    }
    else if (minXv != maxXv)
    {
        if (minVectorX.x == 0.0 && minVectorX.y == 0.0)
        {
            minVectorX = minXv;
            maxVectorX = maxXv;
        }
        else if (minVectorX != maxVectorX)
        {
            const auto avg = glm::normalize(minXv + maxXv + minVectorX + maxVectorX);
            minVectorX = glm::dot(avg, minXv) < glm::dot(avg, minVectorX) ? minXv : minVectorX;
            maxVectorX = glm::dot(avg, maxXv) < glm::dot(avg, maxVectorX) ? maxXv : maxVectorX;
        }
    }

    minCoordY = qMin(minCoordY, minYz);
    maxCoordY = qMax(maxCoordY, maxYz);

    return true;
}
