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
        lp0.x += tilesCount;
    else if (lp0.x - lp1.x >= halfTilesCount)
        lp1.x += tilesCount;
    if (lp1.y - lp0.y >= halfTilesCount)
        lp0.y += tilesCount;
    else if (lp0.y - lp1.y >= halfTilesCount)
        lp1.y += tilesCount;

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
            lp0.x += tilesCount;
        else if (lp0.x - lp1.x >= halfTilesCount)
            lp1.x += tilesCount;
        if (lp1.y - lp0.y >= halfTilesCount)
            lp0.y += tilesCount;
        else if (lp0.y - lp1.y >= halfTilesCount)
            lp1.y += tilesCount;

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

OsmAnd::AreaI OsmAnd::Frustum2D31::getBBox31() const
{
    const auto np0 = Utilities::normalizeCoordinates(p0, ZoomLevel31);
    const auto np1 = Utilities::normalizeCoordinates(p1, ZoomLevel31);
    const auto np2 = Utilities::normalizeCoordinates(p2, ZoomLevel31);
    const auto np3 = Utilities::normalizeCoordinates(p3, ZoomLevel31);

    return AreaI(np0, np0).enlargeToInclude(np1).enlargeToInclude(np2).enlargeToInclude(np3);
}
