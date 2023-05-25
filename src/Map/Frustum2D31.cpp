#include "Frustum2D31.h"

bool OsmAnd::Frustum2D31::test(const PointI& p_) const
{
    AreaI64 bbox;
    bbox.topLeft = bbox.bottomRight = p0;
    bbox.enlargeToInclude(p1);
    bbox.enlargeToInclude(p2);
    bbox.enlargeToInclude(p3);
    const auto tilesCount = static_cast<int64_t>(1ull << ZoomLevel31);
    const auto xMinK = qFloor(static_cast<double>(bbox.left()) / tilesCount);
    const auto xMaxK = qCeil(static_cast<double>(bbox.right()) / tilesCount);
    const auto yMinK = qFloor(static_cast<double>(bbox.top()) / tilesCount);
    const auto yMaxK = qCeil(static_cast<double>(bbox.bottom()) / tilesCount);

    // 3+ repeats for world include any point in world
    if (qAbs(xMaxK - xMinK) >= 3 && qAbs(yMaxK - yMinK) >= 3)
        return true;

    PointI64 dP;
    const PointI64 p(p_);
    for (auto xK = xMinK; xK <= xMaxK; xK++)
    {
        dP.x = tilesCount * xK;
        for (auto yK = yMinK; yK <= yMaxK; yK++)
        {
            dP.y = tilesCount * yK;
            if (Frustum2DI64::test(p + dP))
                return true;
        }
    }

    return false;
}

bool OsmAnd::Frustum2D31::test(const PointI& lp0_, const PointI& lp1_) const
{
    AreaI64 bbox;
    bbox.topLeft = bbox.bottomRight = p0;
    bbox.enlargeToInclude(p1);
    bbox.enlargeToInclude(p2);
    bbox.enlargeToInclude(p3);
    const auto tilesCount = static_cast<int64_t>(1ull << ZoomLevel31);
    const auto xMinK = qFloor(static_cast<double>(bbox.left()) / tilesCount);
    const auto xMaxK = qCeil(static_cast<double>(bbox.right()) / tilesCount);
    const auto yMinK = qFloor(static_cast<double>(bbox.top()) / tilesCount);
    const auto yMaxK = qCeil(static_cast<double>(bbox.bottom()) / tilesCount);

    // 3+ repeats for world include any point in world
    if (qAbs(xMaxK - xMinK) >= 3 && qAbs(yMaxK - yMinK) >= 3)
        return true;

    PointI64 dP;
    PointI64 lp0(lp0_);
    PointI64 lp1(lp1_);

    // Correct transition over the edges of the map
    const auto halfTilesCount = tilesCount >> 1;
    if (lp1.x - lp0.x >= halfTilesCount)
        lp1.x -= tilesCount;
    else if (lp0.x - lp1.x >= halfTilesCount)
        lp0.x -= tilesCount;

    for (auto xK = xMinK; xK <= xMaxK; xK++)
    {
        dP.x = tilesCount * xK;
        for (auto yK = yMinK; yK <= yMaxK; yK++)
        {
            dP.y = tilesCount * yK;
            if (Frustum2DI64::test(lp0 + dP, lp1 + dP))
                return true;
        }
    }

    return false;
}

bool OsmAnd::Frustum2D31::test(const QVector<PointI>& path) const
{
    if (path.isEmpty())
        return false;

    const auto pathSize = path.size();
    if (pathSize == 1)
        return test(path.first());
    if (pathSize == 2)
        return test(path.first(), path.last());

    AreaI64 bbox;
    bbox.topLeft = bbox.bottomRight = p0;
    bbox.enlargeToInclude(p1);
    bbox.enlargeToInclude(p2);
    bbox.enlargeToInclude(p3);
    const auto tilesCount = static_cast<int64_t>(1ull << ZoomLevel31);
    const auto xMinK = qFloor(static_cast<double>(bbox.left()) / tilesCount);
    const auto xMaxK = qCeil(static_cast<double>(bbox.right()) / tilesCount);
    const auto yMinK = qFloor(static_cast<double>(bbox.top()) / tilesCount);
    const auto yMaxK = qCeil(static_cast<double>(bbox.bottom()) / tilesCount);

    // 3+ repeats for world include any point in world
    if (qAbs(xMaxK - xMinK) >= 3 && qAbs(yMaxK - yMinK) >= 3)
        return true;

    PointI64 dP;

    auto pPoint = path.constData();
    auto pPrevPoint = pPoint++;
    for (auto idx = 1; idx < pathSize; idx++)
    {
        PointI64 lp0(*(pPrevPoint++));
        PointI64 lp1(*(pPoint++));

        // Correct transition over the edges of the map
        const auto halfTilesCount = tilesCount >> 1;
        if (lp1.x - lp0.x >= halfTilesCount)
            lp1.x -= tilesCount;
        else if (lp0.x - lp1.x >= halfTilesCount)
            lp0.x -= tilesCount;

        for (auto xK = xMinK; xK <= xMaxK; xK++)
        {
            dP.x = tilesCount * xK;
            for (auto yK = yMinK; yK <= yMaxK; yK++)
            {
                dP.y = tilesCount * yK;
                if (Frustum2DI64::test(lp0 + dP, lp1 + dP))
                    return true;
            }
        }
    }

    return false;
}

bool OsmAnd::Frustum2D31::test(const AreaI& area) const
{    
    AreaI64 bbox;
    bbox.topLeft = bbox.bottomRight = p0;
    bbox.enlargeToInclude(p1);
    bbox.enlargeToInclude(p2);
    bbox.enlargeToInclude(p3);
    const auto tilesCount = static_cast<int64_t>(1ull << ZoomLevel31);
    const auto xMinK = qFloor(static_cast<double>(bbox.left()) / tilesCount);
    const auto xMaxK = qCeil(static_cast<double>(bbox.right()) / tilesCount);
    const auto yMinK = qFloor(static_cast<double>(bbox.top()) / tilesCount);
    const auto yMaxK = qCeil(static_cast<double>(bbox.bottom()) / tilesCount);

    // 3+ repeats for world include any point in world
    if (qAbs(xMaxK - xMinK) >= 3 && qAbs(yMaxK - yMinK) >= 3)
        return true;

    PointI64 dP;
    PointI64 lp0(area.left(), area.top());
    PointI64 lp1(area.right(), area.bottom());
    if (lp1.x < lp0.x)
        lp0.x -= tilesCount;
    if (lp1.y < lp0.y)
        lp0.y -= tilesCount;

    for (auto xK = xMinK; xK <= xMaxK; xK++)
    {
        dP.x = tilesCount * xK;
        for (auto yK = yMinK; yK <= yMaxK; yK++)
        {
            dP.y = tilesCount * yK;
            if (Frustum2DI64::test(AreaI64(lp0 + dP, lp1 + dP)))
                return true;
        }
    }

    return false;
}

OsmAnd::AreaI OsmAnd::Frustum2D31::getBBox31() const
{
    const auto np0 = Utilities::normalizeCoordinates(p0, ZoomLevel31);
    const auto np1 = Utilities::normalizeCoordinates(p1, ZoomLevel31);
    const auto np2 = Utilities::normalizeCoordinates(p2, ZoomLevel31);
    const auto np3 = Utilities::normalizeCoordinates(p3, ZoomLevel31);

    return AreaI(np0, np0).enlargeToInclude(np1).enlargeToInclude(np2).enlargeToInclude(np3);
}

OsmAnd::AreaI OsmAnd::Frustum2D31::getBBoxShifted() const
{
    // Calculate BBox in 32 bit signed coordinates with origin aligned to zero LatLon
    const auto intHalf = INT32_MAX / 2 + 1;
    const PointI shiftToCenter(intHalf, intHalf);
    
    const PointI64 mp0(p0 - shiftToCenter);
    const PointI64 mp1(p1 - shiftToCenter);
    const PointI64 mp2(p2 - shiftToCenter);
    const PointI64 mp3(p3 - shiftToCenter);

    const PointI64 maxCoords(
        std::max(std::max(mp0.x, mp1.x), std::max(mp2.x, mp3.x)),
        std::max(std::max(mp0.y, mp1.y), std::max(mp2.y, mp3.y)));

    const PointI64 offset(maxCoords.x > INT32_MAX ? maxCoords.x - (maxCoords.x & INT32_MAX) : 0,
        maxCoords.y > INT32_MAX ? maxCoords.y - (maxCoords.y & INT32_MAX) : 0);

    const auto np0 = Utilities::wrapCoordinates(mp0 - offset);
    const auto np1 = Utilities::wrapCoordinates(mp1 - offset);
    const auto np2 = Utilities::wrapCoordinates(mp2 - offset);
    const auto np3 = Utilities::wrapCoordinates(mp3 - offset);

    return AreaI(np0, np0).enlargeToInclude(np1).enlargeToInclude(np2).enlargeToInclude(np3);
}
