#ifndef _OSMAND_CORE_MAP_RENDERER_INTERNAL_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_INTERNAL_STATE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    struct MapRendererInternalState
    {
        MapRendererInternalState();
        virtual ~MapRendererInternalState();

    private:
        Q_DISABLE_COPY_AND_MOVE(MapRendererInternalState)
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_INTERNAL_STATE_H_)
