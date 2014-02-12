#ifndef _OSMAND_CORE_FRUSTUM_2D_H_
#define _OSMAND_CORE_FRUSTUM_2D_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "Memory.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    // Points should be specified in CCW order.
    // Axis X: from left to right
    // Axis Y: from top to bottom
    class Frustum2D Q_DECL_FINAL
    {
        OSMAND_USE_MEMORY_MANAGER(Frustum2D);
    private:
        static float crossProduct(const PointF& a, const PointF& b, const PointF& p);
        static bool testLineLineIntersection(const PointF& a0, const PointF& a1, const PointF& b0, const PointF& b1);
        bool isPointInside(const PointF& p) const;
    protected:
    public:
        Frustum2D();
        Frustum2D(const PointF& p0, const PointF& p1, const PointF& p2, const PointF& p3);
        Frustum2D(const Frustum2D& that);
        ~Frustum2D();

        PointF p0;
        PointF p1;
        PointF p2;
        PointF p3;
        
        Frustum2D& operator=(const Frustum2D& that);

        bool test(const PointF& p) const;
        bool test(const PointF& lp0, const PointF& lp1) const;
        bool test(const AreaF& area) const;
    };
}

#endif // !defined(_OSMAND_CORE_FRUSTUM_2D_H_)
