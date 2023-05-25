#ifndef _OSMAND_CORE_FRUSTUM_2D_31_H_
#define _OSMAND_CORE_FRUSTUM_2D_31_H_

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
#include "Frustum2D.h"
#include "Utilities.h"

namespace OsmAnd
{
    struct Frustum2D31 : public Frustum2DI64
    {
        OSMAND_USE_MEMORY_MANAGER(Frustum2D31);

        inline Frustum2D31& operator=(const Frustum2DI64& that)
        {
            this->p0 = that.p0;
            this->p1 = that.p1;
            this->p2 = that.p2;
            this->p3 = that.p3;

            return *this;
        }

        bool test(const PointI& p) const;
        bool test(const PointI& lp0, const PointI& lp1) const;
        bool test(const QVector<PointI>& path) const;
        bool test(const AreaI& area) const;
        AreaI getBBox31() const;
        AreaI getBBoxShifted() const;
    };
}

#endif // !defined(_OSMAND_CORE_FRUSTUM_2D_31_H_)
