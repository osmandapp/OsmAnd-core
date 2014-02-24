#include "OfflineMapDataTile_P.h"
#include "OfflineMapDataTile.h"

#include "MapObject.h"
#include "ObfMapSectionInfo.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::OfflineMapDataTile_P::OfflineMapDataTile_P( OfflineMapDataTile* owner_ )
    : owner(owner_)
{
}

OsmAnd::OfflineMapDataTile_P::~OfflineMapDataTile_P()
{
}

void OsmAnd::OfflineMapDataTile_P::cleanup()
{
    // Release rasterizer context
    _rasterizerContext.reset();

    // Remove tile reference from collection
    if(const auto entry = _refEntry.lock())
    {
        if(const auto link = entry->link.lock())
            link->collection.removeEntry(entry->tileId, entry->zoom);
    }

    // Dereference shared map objects
    if(const auto link = _link.lock())
    {
        // Get bounding box that covers this tile
        const auto tileBBox31 = Utilities::tileBoundingBox31(owner->tileId, owner->zoom);

        for(auto& mapObject : _mapObjects)
        {
            // Skip all map objects that can not be shared
            const auto canNotBeShared = tileBBox31.contains(mapObject->bbox31);
            if(canNotBeShared)
                continue;

            link->provider._sharedMapObjects.releaseReference(mapObject->id, owner->zoom, mapObject);
        }
    }
    _mapObjects.clear();
}
