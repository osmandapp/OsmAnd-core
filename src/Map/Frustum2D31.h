#ifndef _OSMAND_CORE_FRUSTUM_2D_31_H_
#define _OSMAND_CORE_FRUSTUM_2D_31_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "Memory.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    class Frustum2D31 Q_DECL_FINAL
    {
        OSMAND_USE_MEMORY_MANAGER(Frustum2D31);
    private:
        bool isPointInside(const PointI& p) const;
    protected:
    public:
        Frustum2D31();
        Frustum2D31(const PointI& p0, const PointI& p1, const PointI& p2, const PointI& p3);
        Frustum2D31(const Frustum2D31& that);
        ~Frustum2D31();

        PointI p0;
        PointI p1;
        PointI p2;
        PointI p3;
        
        Frustum2D31& operator=(const Frustum2D31& that);

        bool test(const PointI& p) const;
        bool test(const PointI& lp0, const PointI& lp1) const;
        bool test(const AreaI& area) const;
    };
}

#endif // !defined(_OSMAND_CORE_FRUSTUM_2D_31_H_)
