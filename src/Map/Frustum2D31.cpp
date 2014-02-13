#include "Frustum2D31.h"
#include "Utilities.h"

OsmAnd::Frustum2D31::Frustum2D31()
{
}

OsmAnd::Frustum2D31::Frustum2D31(const PointI& p0_, const PointI& p1_, const PointI& p2_, const PointI& p3_)
    : p0(p0_)
    , p1(p1_)
    , p2(p2_)
    , p3(p3_)
{
}

OsmAnd::Frustum2D31::Frustum2D31(const Frustum2D31& that)
    : p0(that.p0)
    , p1(that.p1)
    , p2(that.p2)
    , p3(that.p3)
{
}

OsmAnd::Frustum2D31::~Frustum2D31()
{
}

OsmAnd::Frustum2D31& OsmAnd::Frustum2D31::operator=(const Frustum2D31& that)
{
    p0 = that.p0;
    p1 = that.p1;
    p2 = that.p2;
    p3 = that.p3;

    return *this;
}

bool OsmAnd::Frustum2D31::test(const PointI& p) const
{
    return isPointInside(p);
}

bool OsmAnd::Frustum2D31::test(const PointI& lp0, const PointI& lp1) const
{
    // Check if any of line points is inside.
    // This case covers inner line and partially inner line (that has one vertex inside).
    if(isPointInside(lp0) || isPointInside(lp1))
        return true;

    // Check if line 'lp0-lp1' intersects any of edges.
    // This case covers intersecting line, that has start and stop outside of frustum.
    return
        testLineLineIntersection(lp0, lp1, p0, p1) ||
        testLineLineIntersection(lp0, lp1, p1, p2) ||
        testLineLineIntersection(lp0, lp1, p2, p3) ||
        testLineLineIntersection(lp0, lp1, p3, p0);
}

bool OsmAnd::Frustum2D31::test(const AreaI& area) const
{
    const auto a0 = area.topLeft;
    const auto a1 = area.topRight();
    const auto a2 = area.bottomRight;
    const auto a3 = area.bottomLeft();

    // Check if any vertex of area is inside.
    // This case covers inner area and partially inner area (that has at least 1 vertex inside)
    if(isPointInside(a0) || isPointInside(a1) || isPointInside(a2) || isPointInside(a3))
        return true;

    // Check if any area edge intersects any of frustum edges.
    // This case covers intersecting area that has no vertex inside frustum.
    if(
        testLineLineIntersection(a0, a1, p0, p1) ||
        testLineLineIntersection(a0, a1, p1, p2) ||
        testLineLineIntersection(a0, a1, p2, p3) ||
        testLineLineIntersection(a0, a1, p3, p0) ||
        testLineLineIntersection(a1, a2, p0, p1) ||
        testLineLineIntersection(a1, a2, p1, p2) ||
        testLineLineIntersection(a1, a2, p2, p3) ||
        testLineLineIntersection(a1, a2, p3, p0) ||
        testLineLineIntersection(a2, a3, p0, p1) ||
        testLineLineIntersection(a2, a3, p1, p2) ||
        testLineLineIntersection(a2, a3, p2, p3) ||
        testLineLineIntersection(a2, a3, p3, p0) ||
        testLineLineIntersection(a3, a0, p0, p1) ||
        testLineLineIntersection(a3, a0, p1, p2) ||
        testLineLineIntersection(a3, a0, p2, p3) ||
        testLineLineIntersection(a3, a0, p3, p0))
        return true;

    // Check if frustum is totally inside area.
    return
        area.contains(p0) ||
        area.contains(p1) ||
        area.contains(p2) ||
        area.contains(p3);
}

bool OsmAnd::Frustum2D31::isPointInside(const PointI& p) const
{
    // Check if point 'p' is on the same 'side' of each edge
    const auto sign0 = crossProductSign(p0, p1, p);
    const auto sign1 = crossProductSign(p1, p2, p);
    const auto sign2 = crossProductSign(p2, p3, p);
    const auto sign3 = crossProductSign(p3, p0, p);
    int sign = sign0;
    if(sign1 != 0)
    {
        if(sign != 0 && sign != sign1)
            return false;
        sign = sign1;
    }
    if(sign2 != 0)
    {
        if(sign != 0 && sign != sign2)
            return false;
        sign = sign2;
    }
    if(sign3 != 0)
    {
        if(sign != 0 && sign != sign3)
            return false;
    }
    return true;
}

bool OsmAnd::Frustum2D31::testLineLineIntersection(const PointI& a0, const PointI& a1, const PointI& b0, const PointI& b1)
{
    const auto a1x_a0x = a1.x - a0.x;
    const auto a1y_a0y = a1.y - a0.y;
    const auto b0y_b1y = b0.y - b1.y;
    const auto b0x_b1x = b0.x - b1.x;
    const auto b0x_a0x = b0.x - a0.x;
    const auto b0y_a0y = b0.y - a0.y;

    const auto d_ = static_cast<int64_t>(a1x_a0x*b0y_b1y) - static_cast<int64_t>(b0x_b1x*a1y_a0y);
    const auto t_ = static_cast<int64_t>(b0y_b1y*b0x_a0x) - static_cast<int64_t>(b0x_b1x*b0y_a0y);
    const auto u_ = static_cast<int64_t>(a1x_a0x*b0y_a0y) - static_cast<int64_t>(a1y_a0y*b0x_a0x);

    if(d_ == 0)
    {
        if(t_== 0 && u_ == 0)
            return true;
        return false;
    }
    
    const auto t = (static_cast<double>(t_) / static_cast<double>(d_));
    const auto u = (static_cast<double>(u_) / static_cast<double>(d_));

    return
        t >= 0.0 && t <= 1.0 &&
        u >= 0.0 && u <= 1.0;
}

int OsmAnd::Frustum2D31::crossProductSign(const PointI& a, const PointI& b, const PointI& p)
{
    const int64_t bx_ax = b.x - a.x;
    const int64_t by_ay = b.y - a.y;
    const int64_t px_ax = p.x - a.x;
    const int64_t py_ay = p.y - a.y;
    return Utilities::sign(by_ay*px_ax - bx_ax*py_ay);
}
