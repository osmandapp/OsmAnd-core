#ifndef _OSMAND_CORE_ICU_PRIVATE_H_
#define _OSMAND_CORE_ICU_PRIVATE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"

namespace OsmAnd
{
    namespace ICU
    {
        bool initialize();
        void release();
    }
}

#endif // !defined(_OSMAND_CORE_ICU_PRIVATE_H_)
