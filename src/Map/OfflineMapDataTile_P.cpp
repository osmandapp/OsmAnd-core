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

    // Remove all weak pointers to shared map objects that have become unique
    if(const auto link = _link.lock())
    {
#if defined(DEBUG) || defined(_DEBUG)
        int cleanedObjects = 0;
#endif

        for(auto itMapObject = _mapObjects.cbegin(); itMapObject != _mapObjects.cend(); ++itMapObject)
        {
            const auto& mapObject = *itMapObject;

            // If reference to this object is not unique, skip it
            if(!mapObject.unique())
                continue;

#if defined(DEBUG) || defined(_DEBUG)
            cleanedObjects++;
#endif

            for(int zoom = mapObject->level->minZoom; zoom <= mapObject->level->maxZoom; zoom++)
            {
                auto& cacheLevel = link->provider._mapObjectsCache[zoom];

                {
                    QWriteLocker scopedLocker(&cacheLevel._mutex);

                    cacheLevel._mapObjects.remove(mapObject->id);
                }
            }
        }

#if defined(DEBUG) || defined(_DEBUG)
        LogPrintf(LogSeverityLevel::Info,
            "%d map objects cleaned with %dx%d@%d",
            cleanedObjects,
            owner->tileId.x, owner->tileId.y, owner->zoom);
#endif
    }
    _mapObjects.clear();
}
