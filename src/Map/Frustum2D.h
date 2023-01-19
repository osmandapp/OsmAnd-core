#ifndef _OSMAND_CORE_FRUSTUM_2D_H_
#define _OSMAND_CORE_FRUSTUM_2D_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QVector>
#include "restore_internal_warnings.h"

#include "ignore_warnings_on_external_includes.h"
#include <glm/glm.hpp>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "MemoryCommon.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    template<typename T>
    struct Frustum2D
    {
        OSMAND_USE_MEMORY_MANAGER(Frustum2D);

        typedef Frustum2D<T> Frustum2DT;
        typedef Point<T> PointT;
        typedef Area<T> AreaT;

        PointT p0;
        PointT p1;
        PointT p2;
        PointT p3;
        
        inline bool test(const PointT& p) const
        {
            return isPointInsideArea(p, p0, p1, p2, p3);
        }

        inline bool test(const PointT& lp0, const PointT& lp1) const
        {
            // Check if any of line points is inside.
            // This case covers inner line and partially inner line (that has one vertex inside).
            if (test(lp0) || test(lp1))
                return true;

            // Check if line 'lp0-lp1' intersects any of edges.
            // This case covers intersecting line, that has start and stop outside of frustum.
            return
                testLineLineIntersection(lp0, lp1, p0, p1) ||
                testLineLineIntersection(lp0, lp1, p1, p2) ||
                testLineLineIntersection(lp0, lp1, p2, p3) ||
                testLineLineIntersection(lp0, lp1, p3, p0);
        }

        bool test(const QVector<PointT>& path) const
        {
            if (path.isEmpty())
                return false;

            const auto pathSize = path.size();
            if (pathSize == 1)
                return test(path.first());
            if (pathSize == 2)
                return test(path.first(), path.last());

            auto pPoint = path.constData();
            auto pPrevPoint = pPoint++;
            for (auto idx = 1; idx < pathSize; idx++)
            {
                if (test(*(pPrevPoint++), *(pPoint++)))
                    return true;
            }

            return false;
        }

        bool test(const QVector< glm::tvec2<T, glm::precision::defaultp> >& path) const
        {
            if (path.isEmpty())
                return false;

            const auto pathSize = path.size();
            if (pathSize == 1)
                return test(path.first());
            if (pathSize == 2)
                return test(path.first(), path.last());

            auto pPoint = path.constData();
            auto pPrevPoint = pPoint++;
            for (auto idx = 1; idx < pathSize; idx++)
            {
                if (test(*(pPrevPoint++), *(pPoint++)))
                    return true;
            }

            return false;
        }

        bool test(const AreaT& area) const
        {
            // Check if frustum is partially inside area.
            if (area.contains(p0) || area.contains(p1) || area.contains(p2) || area.contains(p3))
                return true;

            // Check if area is contained or intersects frustum
            if (areaContainedInOrIntersectsArea(area, p0, p1, p2, p3))
                return true;

            return false;
        }

        Frustum2DT operator+(const PointT& shift) const
        {
            Frustum2DT frustum = *this;

            frustum.p0 += shift;
            frustum.p1 += shift;
            frustum.p2 += shift;
            frustum.p3 += shift;

            return frustum;
        }

        Frustum2DT& operator+=(const PointT& shift)
        {
            p0 += shift;
            p1 += shift;
            p2 += shift;
            p3 += shift;

            return *this;
        }

        Frustum2DT operator-(const PointT& shift) const
        {
            Frustum2DT frustum = *this;

            frustum.p0 -= shift;
            frustum.p1 -= shift;
            frustum.p2 -= shift;
            frustum.p3 -= shift;

            return frustum;
        }

        Frustum2DT& operator-=(const PointT& shift)
        {
            p0 -= shift;
            p1 -= shift;
            p2 -= shift;
            p3 -= shift;

            return *this;
        }

        AreaT getBBox() const
        {
            return AreaT(p0, p0).enlargeToInclude(p1).enlargeToInclude(p2).enlargeToInclude(p3);
        }
    };

    typedef Frustum2D<PointI::CoordType> Frustum2DI;
    typedef Frustum2D<PointI64::CoordType> Frustum2DI64;
    typedef Frustum2D<PointF::CoordType> Frustum2DF;
    typedef Frustum2D<PointD::CoordType> Frustum2DD;
}

#endif // !defined(_OSMAND_CORE_FRUSTUM_2D_H_)
