#include "MapPrimitivesProvider_P.h"
#include "MapPrimitivesProvider.h"

//#define OSMAND_PERFORMANCE_METRICS 2
#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif // !defined(OSMAND_PERFORMANCE_METRICS)

#include "IMapObjectsProvider.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"
#include "ObfMapObject.h"

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

QHash<std::shared_ptr<const OsmAnd::MapObject>, QList<std::shared_ptr<const OsmAnd::MapObject>>> OsmAnd::MapPrimitivesProvider_P::retreivePolygons(PointI point, ZoomLevel zoom)
{
    std::shared_ptr<TileEntry> tileEntry;
    TileId tile = OsmAnd::Utilities::getTileId(point, zoom);
    std::shared_ptr<MapPrimitivesProvider::Data> outTiledPrimitives = nullptr;
    QHash<std::shared_ptr<const MapObject>, QList<std::shared_ptr<const MapObject>>> result;
    _tileReferences.obtainOrAllocateEntry(tileEntry, tile, zoom,
        []
        (const TiledEntriesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileEntry*
        {
            return new TileEntry(collection, tileId, zoom);
        });
    // If state is "Undefined", change it to "Loading" and proceed with loading
    if (tileEntry->setStateIf(TileState::Undefined, TileState::Loading))
        return result;

    // In case tile entry is being loaded, wait until it will finish loading
    if (tileEntry->getState() == TileState::Loading)
    {
        QReadLocker scopedLcoker(&tileEntry->loadedConditionLock);
        // If tile is in 'Loading' state, wait until it will become 'Loaded'
        while (tileEntry->getState() != TileState::Loaded)
            REPEAT_UNTIL(tileEntry->loadedCondition.wait(&tileEntry->loadedConditionLock));
    }

    QSet<std::shared_ptr<const MapObject>> polygons;
    if (tileEntry->dataIsPresent)
    {
        // Otherwise, try to lock tile reference
        outTiledPrimitives = tileEntry->dataWeakRef.lock();
        if (outTiledPrimitives && outTiledPrimitives->primitivisedObjects)
        {
            for (const auto & p : outTiledPrimitives->primitivisedObjects->polygons)
            {
                const auto & mapObj = p->sourceObject;
                if (OsmAnd::Utilities::contains(mapObj->points31, point))
                {
                    std::shared_ptr<const ObfMapObject> obfMapObject = std::dynamic_pointer_cast<const ObfMapObject>(mapObj);
                    if (obfMapObject) {
                        polygons.insert(mapObj);
                    }
                }
            }
            for (const std::shared_ptr<const MapObject> & polygon : polygons)
            {
                QList<std::shared_ptr<const MapObject>> points;
                for (const auto & point : outTiledPrimitives->primitivisedObjects->points)
                {
                    const auto & mapObj = point->sourceObject;
                    if (OsmAnd::Utilities::contains(polygon->points31, mapObj->bbox31.topLeft) || OsmAnd::Utilities::contains(polygon->points31, mapObj->bbox31.bottomRight))
                    {
                        points.push_back(mapObj);
                    }
                }
                std::sort(points.begin(), points.end(), [point](const std::shared_ptr<const MapObject> &a, const std::shared_ptr<const MapObject> &b) {
                    int outSegInd0;
                    int outSegInd1;
                    double da = OsmAnd::Utilities::minimalSquaredDistanceToLineSegmentFromPoint(a->points31, point, &outSegInd0, &outSegInd1);
                    double db = OsmAnd::Utilities::minimalSquaredDistanceToLineSegmentFromPoint(b->points31, point, &outSegInd0, &outSegInd1);
                    return da < db;
                });
                result.insert(polygon, points);
            }
        }
    }

    /*test only*/
    /*for (const auto & mapObj : polygons)
    {
        QString s = "";
        for (auto i = mapObj->captions.begin(); i != mapObj->captions.end(); ++i)
        {
            QString c = i.value();
            if (!c.isEmpty()) {
                s.append(c + " ");
            }
        }
        QHash<QString, QString> tags = mapObj->getResolvedAttributes();
        for (auto i = tags.begin(); i != tags.end(); ++i)
        {
            s.append(i.key() + ":" + i.value() + " ");
        }
        LogPrintf(LogSeverityLevel::Info, "POLYGON: %s", qPrintable(s));
        
        const QList<std::shared_ptr<const MapObject>> & points = result.value(mapObj);
        for (const auto & point : points)
        {
            s = "";
            for (auto i = point->captions.begin(); i != point->captions.end(); ++i)
            {
                QString c = i.value();
                if (!c.isEmpty()) {
                    s.append(c + " ");
                }
            }
            QHash<QString, QString> tags = point->getResolvedAttributes();
            for (auto i = tags.begin(); i != tags.end(); ++i)
            {
                s.append(i.key() + ":" + i.value() + " ");
            }
            LogPrintf(LogSeverityLevel::Info, "POINT: %s", qPrintable(s));
        }
        LogPrintf(LogSeverityLevel::Info, "----------------------");
    }*/
    /*test only*/
    return result;
}

bool OsmAnd::MapPrimitivesProvider_P::obtainTiledPrimitives(
    const MapPrimitivesProvider::Request& request,
    std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives,
    MapPrimitivesProvider_Metrics::Metric_obtainData* const metric_)
{
#if OSMAND_PERFORMANCE_METRICS
    MapPrimitivesProvider_Metrics::Metric_obtainData localMetric;
    const auto metric = metric_ ? metric_ : &localMetric;
#else
    const auto metric = metric_;
#endif

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
            QReadLocker scopedLcoker(&tileEntry->loadedConditionLock);

            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while (tileEntry->getState() != TileState::Loaded)
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
            outTiledPrimitives = tileEntry->dataWeakRef.lock();

            // If successfully locked, just return it
            if (outTiledPrimitives)
                return true;

            // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
            // This will enable creation of new entry on next loop cycle
            _tileReferences.removeEntry(request.tileId, request.zoom);
            tileEntry.reset();
        }
    }

    const Stopwatch totalTimeStopwatch(
#if OSMAND_PERFORMANCE_METRICS
        true
#else
        metric != nullptr
#endif // OSMAND_PERFORMANCE_METRICS
        );

    // Obtain map objects data tile
    std::shared_ptr<IMapObjectsProvider::Data> dataTile;
    std::shared_ptr<Metric> submetric;
    owner->mapObjectsProvider->obtainTiledMapObjects(
        request,
        dataTile,
        metric ? &submetric : nullptr);
    if (metric && submetric)
        metric->addOrReplaceSubmetric(submetric);
    if (!dataTile)
    {
        // Store flag that there was no data and mark tile entry as 'Loaded'
        tileEntry->dataIsPresent = false;
        tileEntry->setState(TileState::Loaded);

        // Notify that tile has been loaded
        {
            QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }

        outTiledPrimitives.reset();
        return true;
    }

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
            nullptr,
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
            nullptr,
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
            nullptr,
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
            dataTile->tileSurfaceType,
            dataTile->mapObjects,
            //NOTE: So far it's safe to turn off this cache. But it has to be rewritten. Since lock/unlock occurs too often, this kills entire performance
            //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much. Or use supernodes, like DataBlock
            nullptr, //_primitiviserCache,
            nullptr,
            metric ? metric->findOrAddSubmetricOfType<MapPrimitiviser_Metrics::Metric_primitiviseWithSurface>().get() : nullptr);
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
        QWriteLocker scopedLcoker(&tileEntry->loadedConditionLock);
        tileEntry->loadedCondition.wakeAll();
    }

    if (metric)
        metric->elapsedTime = totalStopwatch.elapsed();

#if OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS <= 1
    LogPrintf(LogSeverityLevel::Info,
        "%d polygons, %d polylines, %d points primitivised from %dx%d@%d in %fs",
        primitivisedObjects->polygons.size(),
        primitivisedObjects->polylines.size(),
        primitivisedObjects->polygons.size(),
        request.tileId.x,
        request.tileId.y,
        request.zoom,
        totalStopwatch.elapsed());
#else
    LogPrintf(LogSeverityLevel::Info,
        "%d polygons, %d polylines, %d points primitivised from %dx%d@%d in %fs:\n%s",
        primitivisedObjects->polygons.size(),
        primitivisedObjects->polylines.size(),
        primitivisedObjects->polygons.size(),
        request.tileId.x,
        request.tileId.y,
        request.zoom,
        totalStopwatch.elapsed(),
        qPrintable(metric ? metric->toString(false, QLatin1String("\t - ")) : QLatin1String("(null)")));
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS
    
    return true;
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
