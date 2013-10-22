#include "OfflineMapDataTile_P.h"
#include "OfflineMapDataTile.h"

#include "MapObject.h"

OsmAnd::OfflineMapDataTile_P::OfflineMapDataTile_P( OfflineMapDataTile* owner_ )
    : owner(owner_)
{
}

OsmAnd::OfflineMapDataTile_P::~OfflineMapDataTile_P()
{
}

void OsmAnd::OfflineMapDataTile_P::cleanup()
{
    // Remove tile reference from collection
    if(const auto entry = _refEntry.lock())
    {
        if(const auto link = entry->link.lock())
            link->collection.removeEntry(entry);
    }

    // Remove all weak pointers to unique map objects from data cache
    if(const auto link = _link.lock())
    {
        for(int zoom = MinZoomLevel; zoom <= MaxZoomLevel; zoom++)
        {
            auto& dataCache = link->provider._dataCache[zoom];
            QWriteLocker scopedLocker(&dataCache._mapObjectsMutex);

            for(auto itMapObject = owner->mapObjects.cbegin(); itMapObject != owner->mapObjects.cend(); ++itMapObject)
            {
                const auto& mapObject = *itMapObject;

                // If reference to this object is not unique, skip it
                if(!mapObject.unique())
                    continue;

                dataCache._mapObjects.remove(OfflineMapDataProvider_P::makeInternalId(mapObject));
            }
        }
    }
}
