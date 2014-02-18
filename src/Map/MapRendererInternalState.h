#ifndef _OSMAND_CORE_MAP_RENDERER_INTERNAL_STATE_H_
#define _OSMAND_CORE_MAP_RENDERER_INTERNAL_STATE_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include <QList>

#include "OsmAndCore.h"
#include "CommonTypes.h"

namespace OsmAnd
{
    struct MapRendererInternalState
    {
        MapRendererInternalState();
        virtual ~MapRendererInternalState();

        TileId targetTileId;
        PointF targetInTileOffsetN;
        QList<TileId> visibleTiles;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_INTERNAL_STATE_H_)
