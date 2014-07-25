#include "BinaryMapPrimitivesProvider_P.h"
#include "BinaryMapPrimitivesProvider.h"

#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include "BinaryMapDataProvider.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::BinaryMapPrimitivesProvider_P::BinaryMapPrimitivesProvider_P(BinaryMapPrimitivesProvider* owner_)
    : _primitiviserCache(new Primitiviser::Cache())
    , owner(owner_)
{
}

OsmAnd::BinaryMapPrimitivesProvider_P::~BinaryMapPrimitivesProvider_P()
{
}

bool OsmAnd::BinaryMapPrimitivesProvider_P::obtainData(
    const TileId tileId,
    const ZoomLevel zoom,
    std::shared_ptr<MapTiledData>& outTiledData,
    BinaryMapPrimitivesProvider_Metrics::Metric_obtainData* const metric_,
    const IQueryController* const queryController)
{
#if OSMAND_PERFORMANCE_METRICS
    BinaryMapPrimitivesProvider_Metrics::Metric_obtainData localMetric;
    const auto metric = metric_ ? metric_ : &localMetric;
#else
    const auto metric = metric_;
#endif

    const Stopwatch totalStopwatch(metric != nullptr);

    std::shared_ptr<TileEntry> tileEntry;

    for (;;)
    {
        // Try to obtain previous instance of tile
        _tileReferences.obtainOrAllocateEntry(tileEntry, tileId, zoom,
            []
            (const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileEntry*
            {
                return new TileEntry(collection, tileId, zoom);
            });

        // If state is "Undefined", change it to "Loading" and proceed with loading
        if (tileEntry->setStateIf(TileState::Undefined, TileState::Loading))
            break;

        // In case tile entry is being loaded, wait until it will finish loading
        if (tileEntry->getState() == TileState::Loading)
        {
            QReadLocker scopedLcoker(&tileEntry->_loadedConditionLock);

            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while (tileEntry->getState() != TileState::Loaded)
                REPEAT_UNTIL(tileEntry->_loadedCondition.wait(&tileEntry->_loadedConditionLock));
        }

        // Try to lock tile reference
        outTiledData = tileEntry->_tile.lock();

        // If successfully locked, just return it
        if (outTiledData)
            return true;

        // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
        // This will enable creation of new entry on next loop cycle
        _tileReferences.removeEntry(tileId, zoom);
        tileEntry.reset();
    }

    const Stopwatch totalTimeStopwatch(
#if OSMAND_PERFORMANCE_METRICS
        true
#else
        metric != nullptr
#endif // OSMAND_PERFORMANCE_METRICS
        );

    // Obtain offline map data tile
    std::shared_ptr<MapTiledData> dataTile_;
    owner->dataProvider->obtainData(
        tileId,
        zoom,
        dataTile_,
        metric ? &metric->obtainBinaryMapDataMetric : nullptr);
    if (!dataTile_)
    {
        outTiledData.reset();
        return true;
    }
    const auto dataTile = std::static_pointer_cast<BinaryMapDataTile>(dataTile_);

    // Get primitivised area
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);
    const auto primitivisedArea = owner->primitiviser->primitivise(
        tileBBox31,
        PointI(owner->tileSize, owner->tileSize),
        zoom,
        dataTile->tileFoundation,
        dataTile->mapObjects,
        nullptr, //_primitiviserCache,
        nullptr,
        metric ? &metric->primitiviseMetric : nullptr);

    // Create tile
    const std::shared_ptr<BinaryMapPrimitivesTile> newTile(new BinaryMapPrimitivesTile(
        dataTile,
        primitivisedArea,
        tileId,
        zoom));
    newTile->_p->_refEntry = tileEntry;

    // Publish new tile
    outTiledData = newTile;

    // Store weak reference to new tile and mark it as 'Loaded'
    tileEntry->_tile = newTile;
    tileEntry->setState(TileState::Loaded);

    // Notify that tile has been loaded
    {
        QWriteLocker scopedLcoker(&tileEntry->_loadedConditionLock);
        tileEntry->_loadedCondition.wakeAll();
    }

    if (metric)
        metric->elapsedTime = totalStopwatch.elapsed();

#if OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS <= 1
    LogPrintf(LogSeverityLevel::Info,
        "%d polygons, %d polylines, %d points primitivised from %dx%d@%d in %fs",
        primitivisedArea->polygons.size(),
        primitivisedArea->polylines.size(),
        primitivisedArea->polygons.size(),
        tileId.x,
        tileId.y,
        zoom,
        totalStopwatch.elapsed());
#else
    LogPrintf(LogSeverityLevel::Info,
        "%d polygons, %d polylines, %d points primitivised from %dx%d@%d in %fs:\n%s",
        primitivisedArea->polygons.size(),
        primitivisedArea->polylines.size(),
        primitivisedArea->polygons.size(),
        tileId.x,
        tileId.y,
        zoom,
        totalStopwatch.elapsed(),
        qPrintable(metric ? metric->toString(QLatin1String("\t - ")) : QLatin1String("(null)")));
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS
    
    return true;
}

OsmAnd::BinaryMapPrimitivesTile_P::BinaryMapPrimitivesTile_P(BinaryMapPrimitivesTile* owner_)
    : owner(owner_)
{
}

OsmAnd::BinaryMapPrimitivesTile_P::~BinaryMapPrimitivesTile_P()
{
}

void OsmAnd::BinaryMapPrimitivesTile_P::cleanup()
{
    // Remove tile reference from collection. All checks here does not matter,
    // since entry->tile reference is already expired (execution is already in destructor of OfflineMapDataTile!)
    if (const auto entry = _refEntry.lock())
    {
        if (const auto link = entry->link.lock())
            link->collection.removeEntry(entry->tileId, entry->zoom);
    }
}
