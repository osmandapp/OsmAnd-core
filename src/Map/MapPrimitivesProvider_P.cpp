#include "MapPrimitivesProvider_P.h"
#include "MapPrimitivesProvider.h"

#include "IMapObjectsProvider.h"
#include "Stopwatch.h"
#include "SimpleQueryController.h"
#include "Utilities.h"
#include "Logging.h"
#include "MapRendererPerformanceMetrics.h"
#include "MapRasterLayerCache.h"

OsmAnd::MapPrimitivesProvider_P::MapPrimitivesProvider_P(MapPrimitivesProvider* owner_)
    : _primitiviserCache(new MapPrimitiviser::Cache())
    , owner(owner_)
{
}

OsmAnd::MapPrimitivesProvider_P::~MapPrimitivesProvider_P()
{
}

std::shared_ptr<OsmAnd::MapRasterLayerCache> OsmAnd::MapPrimitivesProvider_P::getRasterCache() const
{
    QMutexLocker scopedLocker(&_rasterCacheMutex);
    return _rasterCache;
}

void OsmAnd::MapPrimitivesProvider_P::setRasterCache(const std::shared_ptr<MapRasterLayerCache>& cache)
{
    std::shared_ptr<MapRasterLayerCache> previous;
    {
        QMutexLocker scopedLocker(&_rasterCacheMutex);
        previous = std::move(_rasterCache);
        _rasterCache = cache;
        _rasterHits.clear();
    }
}

void OsmAnd::MapPrimitivesProvider_P::clearRasterCache(const std::shared_ptr<MapRasterLayerCache>& cache)
{
    std::shared_ptr<MapRasterLayerCache> previous;
    {
        QMutexLocker scopedLocker(&_rasterCacheMutex);
        if (_rasterCache == cache)
        {
            previous = std::move(_rasterCache);
            _rasterHits.clear();
        }
    }
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

OsmAnd::MapPrimitivesProvider_P::RasterHitMetadata::RasterHitMetadata(
    const std::shared_ptr<MapPrimitivesProvider_P>& provider_, const std::shared_ptr<MapRasterLayerCache>& cache,
    const MapPrimitivesProvider::Request& source)
    : provider(provider_)
{
    auto copy = std::make_shared<MapPrimitivesProvider::Request>();
    MapPrimitivesProvider::Request::copy(*copy, source);
    request = std::move(copy);
    QMutexLocker lock(&provider_->_rasterCacheMutex);
    if (provider_->_rasterCache == cache)
        provider_->_rasterHits.insert(qMakePair(int(source.zoom), quint64(source.tileId.id)), request);
}

OsmAnd::MapPrimitivesProvider_P::RasterHitMetadata::~RasterHitMetadata()
{
    if (const auto p = provider.lock())
    {
        QMutexLocker lock(&p->_rasterCacheMutex);
        p->_rasterHits.remove(qMakePair(int(request->zoom), quint64(request->tileId.id)), request);
    }
}

QList<std::shared_ptr<const OsmAnd::MapObject>> OsmAnd::MapPrimitivesProvider_P::retreivePolygons(PointI point, ZoomLevel zoom)
{
    std::shared_ptr<TileEntry> tileEntry;
    TileId tile = OsmAnd::Utilities::getTileId(point, zoom);
    std::shared_ptr<MapPrimitivesProvider::Data> outTiledPrimitives = nullptr;
    QList<std::shared_ptr<const OsmAnd::MapObject>> polygons;
    RasterRequest rasterRequest;
    {
        QMutexLocker lock(&_rasterCacheMutex);
        rasterRequest = _rasterHits.value(qMakePair(int(zoom), quint64(tile.id)));
    }
    if (rasterRequest)
    {
        MapPrimitivesProvider::Request request;
        MapPrimitivesProvider::Request::copy(request, *rasterRequest);
        request.queryController = std::make_shared<SimpleQueryController>();
        loadPrimitives(request, outTiledPrimitives, nullptr, true);
    }
    else
    {
        for (const auto* references : { &_tileReferences, &_symbolTileReferences })
        {
            if (references->obtainEntry(tileEntry, tile, zoom) && tileEntry->getState() == TileState::Loaded)
            {
                outTiledPrimitives = tileEntry->dataWeakRef.lock();
                if (outTiledPrimitives)
                    break;
            }
        }
    }
    if (outTiledPrimitives && outTiledPrimitives->primitivisedObjects)
    {
        const auto candidates = outTiledPrimitives->primitivisedObjects->getPolygonObjects();
        for (const auto& object : candidates)
        {
            if (Utilities::contains(object->points31, point))
                polygons.push_back(object);
        }

        // Sort polygons by area.
        std::sort(polygons.begin(), polygons.end(),
            [](const std::shared_ptr<const MapObject> &a, const std::shared_ptr<const MapObject> &b) {
            double da = Utilities::polygonArea(a->points31);
            double db = Utilities::polygonArea(b->points31);
            return da < db;
        });
    }
    return polygons;
}

bool OsmAnd::MapPrimitivesProvider_P::obtainTiledPrimitives(
    const MapPrimitivesProvider::Request& request,
    std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives,
    MapPrimitivesProvider_Metrics::Metric_obtainData* const metric)
{
    return loadPrimitives(request, outTiledPrimitives, metric, request.allowSymbolsOnly);
}

bool OsmAnd::MapPrimitivesProvider_P::loadPrimitives(
    const MapPrimitivesProvider::Request& request,
    std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives,
    MapPrimitivesProvider_Metrics::Metric_obtainData* const metric, bool allowSymbolsOnly)
{
    const auto& queryController = request.queryController;

    if (queryController->isAborted())
        return false;

    const Stopwatch totalStopwatch(metric != nullptr);

    std::shared_ptr<TileEntry> tileEntry;
    auto rasterCache = getRasterCache();
    auto rasterCacheKey = rasterCache ? rasterCache->key(request) : QByteArray();
    const bool useSymbolsCache = allowSymbolsOnly && !rasterCacheKey.isEmpty();
    if (useSymbolsCache && _tileReferences.obtainEntry(tileEntry, request.tileId, request.zoom)
        && tileEntry->getState() == TileState::Loaded && tileEntry->matches(rasterCache, rasterCacheKey, request))
    {
        // Symbols can reuse a full graph already held by the raster or 3D provider.
        outTiledPrimitives = tileEntry->dataWeakRef.lock();
        if (outTiledPrimitives || !tileEntry->dataIsPresent)
            return true;
    }
    auto& references = useSymbolsCache ? _symbolTileReferences : _tileReferences;
    const auto detailedZoom = request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom;

    for (;;)
    {
        // Try to obtain previous instance of tile
        references.obtainOrAllocateEntry(tileEntry, request.tileId, request.zoom,
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
            rasterCache = getRasterCache();
            rasterCacheKey = rasterCache ? rasterCache->key(request) : QByteArray();
        }

        if (tileEntry->getState() == TileState::Loaded && tileEntry->matches(rasterCache, rasterCacheKey, request))
        {
            if (!tileEntry->dataIsPresent)
            {
                outTiledPrimitives.reset();
                return true;
            }
            outTiledPrimitives = tileEntry->dataWeakRef.lock();
            if (outTiledPrimitives)
                return true;
        }

        // Remove only this entry: another request may have already replaced it.
        references.removeEntry(request.tileId, request.zoom,
            [&tileEntry](const std::shared_ptr<TileEntry>& entry) { return entry == tileEntry; });
        tileEntry.reset();
        outTiledPrimitives.reset();
        if (queryController->isAborted())
            return false;
    }

    const auto cancelEntry = [&]()
    {
        tileEntry->setState(TileState::Cancelled);
        {
            QWriteLocker lock(&tileEntry->loadedConditionLock);
            tileEntry->loadedCondition.wakeAll();
        }
        references.removeEntry(request.tileId, request.zoom,
            [&tileEntry](const std::shared_ptr<TileEntry>& entry) { return entry == tileEntry; });
    };
    if (queryController->isAborted())
    {
        cancelEntry();
        return false;
    }

    tileEntry->rasterCache = rasterCache;
    tileEntry->rasterCacheKey = rasterCacheKey;
    tileEntry->detailedZoom = detailedZoom;
    std::shared_ptr<IMapObjectsProvider::Data> dataTile;
    std::shared_ptr<MapPrimitiviser::PrimitivisedObjects> primitivisedObjects;
    if (useSymbolsCache)
    {
        if (!rasterCache || rasterCacheKey.isEmpty()
            || !rasterCache->readPrimitives(request, rasterCacheKey, dataTile, primitivisedObjects))
        {
            // Join the full OBF load so concurrent raster and symbol misses share one graph.
            const auto success = loadPrimitives(request, outTiledPrimitives, metric, false);
            cancelEntry();
            return success;
        }
    }
    else
    {
        // Obtain map objects data tile
        std::shared_ptr<Metric> submetric;
        owner->mapObjectsProvider->obtainTiledMapObjects(
            request,
            dataTile,
            metric ? &submetric : nullptr);
        if (metric && submetric)
            metric->addOrReplaceSubmetric(submetric);

        if (queryController->isAborted())
        {
            cancelEntry();
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
        if (owner->mode == MapPrimitivesProvider::Mode::AllObjectsWithoutPolygonFiltering)
        {
            primitivisedObjects = owner->primitiviser->primitiviseAllMapObjects(
                request.zoom,
                request.tileId,
                dataTile->mapObjects,
                //NOTE: So far it's safe to turn off this cache. But it has to be rewritten.
                // Since lock/unlock occurs too often, this kills entire performance
                //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much.
                // Or use supernodes, like DataBlock
                nullptr, //_primitiviserCache,
                queryController,
                metric ? metric->findOrAddSubmetricOfType<
                    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects>().get() : nullptr);
        }
        else if (owner->mode == MapPrimitivesProvider::Mode::AllObjectsWithPolygonFiltering)
        {
            primitivisedObjects = owner->primitiviser->primitiviseAllMapObjects(
                Utilities::getScaleDivisor31ToPixel(PointI(owner->tileSize, owner->tileSize), request.zoom),
                request.zoom,
                request.tileId,
                dataTile->mapObjects,
                //NOTE: So far it's safe to turn off this cache. But it has to be rewritten.
                // Since lock/unlock occurs too often, this kills entire performance
                //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much.
                // Or use supernodes, like DataBlock
                nullptr, //_primitiviserCache,
                queryController,
                metric ? metric->findOrAddSubmetricOfType<
                    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects>().get() : nullptr);
        }
        else if (owner->mode == MapPrimitivesProvider::Mode::WithoutSurface)
        {
            primitivisedObjects = owner->primitiviser->primitiviseWithoutSurface(
                Utilities::getScaleDivisor31ToPixel(PointI(owner->tileSize, owner->tileSize), request.zoom),
                request.zoom,
                request.tileId,
                dataTile->mapObjects,
                //NOTE: So far it's safe to turn off this cache. But it has to be rewritten.
                // Since lock/unlock occurs too often, this kills entire performance
                //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much.
                // Or use supernodes, like DataBlock
                nullptr, //_primitiviserCache,
                queryController,
                metric ? metric->findOrAddSubmetricOfType<
                    MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface>().get() : nullptr);
        }
        else // if (owner->mode == MapPrimitivesProvider::Mode::WithSurface)
        {
            const auto tileBBox31 = Utilities::tileBoundingBox31(request.tileId, request.zoom);
            primitivisedObjects = owner->primitiviser->primitiviseWithSurface(
                tileBBox31,
                PointI(owner->tileSize, owner->tileSize),
                request.zoom,
                request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom,
                request.tileId,
                request.visibleArea31,
                request.areaTime,
                dataTile->tileSurfaceType,
                dataTile->mapObjects,
                //NOTE: So far it's safe to turn off this cache. But it has to be rewritten.
                // Since lock/unlock occurs too often, this kills entire performance
                //NOTE: Maybe a QuadTree-based cache with leaf-only locking will save up much.
                // Or use supernodes, like DataBlock
                nullptr, //_primitiviserCache,
                queryController,
                metric ? metric->findOrAddSubmetricOfType<
                    MapPrimitiviser_Metrics::Metric_primitiviseWithSurface>().get() : nullptr);
        }
    }

    if (queryController->isAborted())
    {
        cancelEntry();
        return false;
    }

    // Create tile
    const std::shared_ptr<MapPrimitivesProvider::Data> newTiledData(new MapPrimitivesProvider::Data(
        request.tileId,
        request.zoom,
        detailedZoom,
        dataTile,
        primitivisedObjects,
        new RetainableCacheMetadata(tileEntry, dataTile->retainableCacheMetadata)));
    newTiledData->rasterCache = rasterCache;
    newTiledData->rasterCacheKey = rasterCacheKey;

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

    if (!useSymbolsCache && OsmAnd::isPerformanceMetricsEnabled())
        OsmAnd::getPerformanceMetrics().primitivesFinish(request.tileId, request.zoom, primitivisedObjects->polygons.size(), primitivisedObjects->polylines.size(), primitivisedObjects->points.size(), metric);

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
            link->collection.removeEntry(tileEntry->tileId, tileEntry->zoom,
                [&tileEntry](const std::shared_ptr<TileEntry>& entry) { return entry == tileEntry; });
    }
}
