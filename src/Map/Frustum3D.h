#ifndef _OSMAND_CORE_FRUSTUM_3D_H_
#define _OSMAND_CORE_FRUSTUM_3D_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "Memory.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    class Frustum3D Q_DECL_FINAL
    {
        OSMAND_USE_MEMORY_MANAGER(Frustum3D);
    private:
    protected:
    public:
        Frustum3D();
        ~Frustum3D();
    };
}

#endif // !defined(_OSMAND_CORE_FRUSTUM_3D_H_)
