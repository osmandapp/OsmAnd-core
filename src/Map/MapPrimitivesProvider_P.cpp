#include "MapPrimitivesProvider_P.h"
#include "MapPrimitivesProvider.h"

#include "IMapObjectsProvider.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"
#include "ObfMapObject.h"
#include "MapRendererPerformanceMetrics.h"

OsmAnd::MapPrimitivesProvider_P::MapPrimitivesProvider_P(MapPrimitivesProvider* owner_)
    : _primitiviserCache(new MapPrimitiviser::Cache())
    , owner(owner_)
{
}

OsmAnd::MapPrimitivesProvider_P::~MapPrimitivesProvider_P()
{
}

bool OsmAnd::MapPrimitivesProvider_P::obtainData(
    const IMapDataProvider::Request& request,
    std::shared_ptr<IMapDataProvider::Data>& outData,
    std::shared_ptr<Metric>* const pOutMetric)
{
    if (pOutMetric)
    {
        if (!pOutMetric->get() || !dynamic_cast<MapPrimitivesProvider_Metrics::Metric_obtainData*>(pOutMetric->get()))
            pOutMetric->reset(new MapPrimitivesProvider_Metrics::Metric_obtainData());
        else
            pOutMetric->get()->reset();
    }

    std::shared_ptr<MapPrimitivesProvider::Data> data;
    const auto result = obtainTiledPrimitives(
        request,
        data,
        pOutMetric ? static_cast<MapPrimitivesProvider_Metrics::Metric_obtainData*>(pOutMetric->get()) : nullptr);
    outData = data;
    return result;
}

QList<std::shared_ptr<const OsmAnd::MapObject>> OsmAnd::MapPrimitivesProvider_P::retreivePolygons(PointI point, ZoomLevel zoom)
{
    std::shared_ptr<TileEntry> tileEntry;
    TileId tile = OsmAnd::Utilities::getTileId(point, zoom);
    std::shared_ptr<MapPrimitivesProvider::Data> outTiledPrimitives = nullptr;
    QList<std::shared_ptr<const OsmAnd::MapObject>> polygons;
    _tileReferences.obtainOrAllocateEntry(tileEntry, tile, zoom,
        []
        (const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileEntry*
        {
            return new TileEntry(collection, tileId, zoom);
        });
    if (tileEntry->getState() != TileState::Loaded)
    {
        return polygons;
    }

    if (tileEntry->dataIsPresent)
    {
        // Otherwise, try to lock tile reference
        outTiledPrimitives = tileEntry->dataWeakRef.lock();
        if (outTiledPrimitives && outTiledPrimitives->primitivisedObjects)
        {
            for (const auto & p : outTiledPrimitives->primitivisedObjects->polygons)
            {
                collectPolygons(polygons, p->sourceObject, p->type, point);
            }
            for (const auto & p : outTiledPrimitives->primitivisedObjects->polylines)
            {
                collectPolygons(polygons, p->sourceObject, p->type, point);
            }
            
            //sort by polygons area
            std::sort(polygons.begin(), polygons.end(), [](const std::shared_ptr<const MapObject> &a, const std::shared_ptr<const MapObject> &b) {
                double da = Utilities::polygonArea(a->points31);
                double db = Utilities::polygonArea(b->points31);
                return da < db;
            });
        }
    }
    return polygons;
}

void OsmAnd::MapPrimitivesProvider_P::collectPolygons(QList<std::shared_ptr<const OsmAnd::MapObject>> & polygons, 
                                                      const std::shared_ptr<const MapObject> & mapObj,
                                                      const MapPrimitiviser::PrimitiveType & type, const PointI & point)
{
    if (type == MapPrimitiviser::PrimitiveType::Polygon)
    {
        if (OsmAnd::Utilities::contains(mapObj->points31, point))
        {
            std::shared_ptr<const ObfMapObject> obfMapObject = std::dynamic_pointer_cast<const ObfMapObject>(mapObj);
            if (obfMapObject && !polygons.contains(mapObj)) 
            {
                polygons.push_back(mapObj);
            }
        }
    }
}

bool OsmAnd::MapPrimitivesProvider_P::obtainTiledPrimitives(
    const MapPrimitivesProvider::Request& request,
    std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives,
    MapPrimitivesProvider_Metrics::Metric_obtainData* const metric)
{
    const auto& queryController = request.queryController;

    if (queryController->isAborted())
        return false;

    const auto empty =
        []
        (const std::shared_ptr<TileEntry>& entry) -> bool
        {
            return !entry->dataWeakRef.lock();
        };

    const Stopwatch totalStopwatch(metric != nullptr);

    std::shared_ptr<TileEntry> tileEntry;

    for (;;)
    {
        // Try to obtain previous instance of tile
        _tileReferences.obtainOrAllocateEntry(tileEntry, request.tileId, request.zoom,
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
            QReadLocker scopedLocker(&tileEntry->loadedConditionLock);

            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while (tileEntry->getState() == TileState::Loading)
                REPEAT_UNTIL(tileEntry->loadedCondition.wait(&tileEntry->loadedConditionLock));
        }

        if (!tileEntry->dataIsPresent)
        {
            // If there was no data, return same
            outTiledPrimitives.reset();
            return true;
        }
        else
        {
            // Otherwise, try to lock tile reference
            if (tileEntry->getState() == TileState::Loaded)
                outTiledPrimitives = tileEntry->dataWeakRef.lock();

            // If successfully locked, just return it
            if (outTiledPrimitives)
                return true;

            // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
            // This will enable creation of new entry on next loop cycle
            _tileReferences.removeEntry(request.tileId, request.zoom, empty);
            tileEntry.reset();

            if (queryController->isAborted())
                return false;
        }
    }

    if (queryController->isAborted())
    {
        tileEntry->setState(TileState::Cancelled);
        {
            QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }
        _tileReferences.removeEntry(request.tileId, request.zoom, empty);
        tileEntry.reset();

        return false;
    }

    // Obtain map objects data tile
    std::shared_ptr<IMapObjectsProvider::Data> dataTile;
    std::shared_ptr<Metric> submetric;
    owner->mapObjectsProvider->obtainTiledMapObjects(
        request,
        dataTile,
        metric ? &submetric : nullptr);
    if (metric && submetric)
        metric->addOrReplaceSubmetric(submetric);

    if (queryController->isAborted())
    {
        tileEntry->setState(TileState::Cancelled);
        {
            QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }
        _tileReferences.removeEntry(request.tileId, request.zoom, empty);
        tileEntry.reset();

        return false;
    }

    if (!dataTile)
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        // Notify that tile has been loaded
        {
            QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }

        outTiledPrimitives.reset();
        return true;
    }

    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().primitivesStart(request.tileId);

    // Get primitivised objects
    std::shared_ptr<MapPrimitiviser::PrimitivisedObjects> primitivisedObjects;
    if (owner->mode == MapPrimitivesProvider::Mode::AllObjectsWithoutPolygonFiltering)
    {
        primitivisedObjects = owner->primitiviser->primitiviseAllMapObjects(
            request.zoom,
            request.tileId,
            dataTile->mapObjects,
            //NOTE: So far it's safe to turn off this cache. But it has to be rewritten. Since lock/unlock occurs too often, this kills entire performance
            //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much. Or use supernodes, like DataBlock
            nullptr, //_primitiviserCache,
            queryController,
            metric ? metric->findOrAddSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects>().get() : nullptr);
    }
    else if (owner->mode == MapPrimitivesProvider::Mode::AllObjectsWithPolygonFiltering)
    {
        primitivisedObjects = owner->primitiviser->primitiviseAllMapObjects(
            Utilities::getScaleDivisor31ToPixel(PointI(owner->tileSize, owner->tileSize), request.zoom),
            request.zoom,
            request.tileId,
            dataTile->mapObjects,
            //NOTE: So far it's safe to turn off this cache. But it has to be rewritten. Since lock/unlock occurs too often, this kills entire performance
            //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much. Or use supernodes, like DataBlock
            nullptr, //_primitiviserCache,
            queryController,
            metric ? metric->findOrAddSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects>().get() : nullptr);
    }
    else if (owner->mode == MapPrimitivesProvider::Mode::WithoutSurface)
    {
        primitivisedObjects = owner->primitiviser->primitiviseWithoutSurface(
            Utilities::getScaleDivisor31ToPixel(PointI(owner->tileSize, owner->tileSize), request.zoom),
            request.zoom,
            request.tileId,
            dataTile->mapObjects,
            //NOTE: So far it's safe to turn off this cache. But it has to be rewritten. Since lock/unlock occurs too often, this kills entire performance
            //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much. Or use supernodes, like DataBlock
            nullptr, //_primitiviserCache,
            queryController,
            metric ? metric->findOrAddSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface>().get() : nullptr);
    }
    else // if (owner->mode == MapPrimitivesProvider::Mode::WithSurface)
    {
        const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);
        primitivisedObjects = owner->primitiviser->primitiviseWithSurface(
            tileBBox31,
            PointI(owner->tileSize, owner->tileSize),
            request.zoom,
            request.tileId,
            request.visibleArea31,
            request.areaTime,
            dataTile->tileSurfaceType,
            dataTile->mapObjects,
            //NOTE: So far it's safe to turn off this cache. But it has to be rewritten. Since lock/unlock occurs too often, this kills entire performance
            //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much. Or use supernodes, like DataBlock
            nullptr, //_primitiviserCache,
            queryController,
            metric ? metric->findOrAddSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitiviseWithSurface>().get() : nullptr);
    }

    if (queryController->isAborted())
    {
        tileEntry->setState(TileState::Cancelled);
        {
            QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }
        _tileReferences.removeEntry(request.tileId, request.zoom, empty);
        tileEntry.reset();

        return false;
    }

    // Create tile
    const std::shared_ptr<MapPrimitivesProvider::Data> newTiledData(new MapPrimitivesProvider::Data(
        request.tileId,
        request.zoom,
        dataTile,
        primitivisedObjects,
        new RetainableCacheMetadata(tileEntry, dataTile->retainableCacheMetadata)));

    // Publish new tile
    outTiledPrimitives = newTiledData;

    // Store weak reference to new tile and mark it as 'Loaded'
    tileEntry->dataIsPresent = true;
    tileEntry->dataWeakRef = newTiledData;
    tileEntry->setState(TileState::Loaded);

    // Notify that tile has been loaded
    {
        QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
        tileEntry->loadedCondition.wakeAll();
    }

    if (metric)
        metric->elapsedTime = totalStopwatch.elapsed();

    if (OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().primitivesFinish(request.tileId, request.zoom, primitivisedObjects->polygons.size(), primitivisedObjects->polylines.size(), primitivisedObjects->points.size(), metric);

    return true;
}

void OsmAnd::MapPrimitivesProvider_P::clearEntriesCache()
{
    _tileReferences.removeAllEntries();
}

OsmAnd::MapPrimitivesProvider_P::RetainableCacheMetadata::RetainableCacheMetadata(
    const std::shared_ptr<TileEntry>& tileEntry,
    const std::shared_ptr<const IMapDataProvider::RetainableCacheMetadata>& binaryMapRetainableCacheMetadata_)
    : tileEntryWeakRef(tileEntry)
    , binaryMapRetainableCacheMetadata(binaryMapRetainableCacheMetadata_)
{
}

OsmAnd::MapPrimitivesProvider_P::RetainableCacheMetadata::~RetainableCacheMetadata()
{
    // Remove tile reference from collection. All checks here does not matter,
    // since entry->tile reference is already expired (execution is already in destructor of OfflineMapDataTile!)
    if (const auto tileEntry = tileEntryWeakRef.lock())
    {
        if (const auto link = tileEntry->link.lock())
            link->collection.removeEntry(tileEntry->tileId, tileEntry->zoom);
    }
}
