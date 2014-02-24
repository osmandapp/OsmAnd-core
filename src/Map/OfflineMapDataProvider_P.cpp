#include "OfflineMapDataProvider_P.h"
#include "OfflineMapDataProvider.h"

#if !defined(OSMAND_PERFORMANCE_METRICS)
#   define OSMAND_PERFORMANCE_METRICS 0
#endif

#include <cassert>
#if OSMAND_PERFORMANCE_METRICS
#   include <chrono>
#endif // OSMAND_PERFORMANCE_METRICS

#include "OfflineMapDataTile.h"
#include "OfflineMapDataTile_P.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "ObfMapSectionInfo.h"
#if OSMAND_PERFORMANCE_METRICS
#   include "ObfMapSectionReader_Metrics.h"
#   include "Rasterizer_Metrics.h"
#endif // OSMAND_PERFORMANCE_METRICS
#include "MapObject.h"
#include "Rasterizer.h"
#include "Utilities.h"
#include "Logging.h"

OsmAnd::OfflineMapDataProvider_P::OfflineMapDataProvider_P( OfflineMapDataProvider* owner_ )
    : owner(owner_)
    , _link(new Link(*this))
{
}

OsmAnd::OfflineMapDataProvider_P::~OfflineMapDataProvider_P()
{
}

void OsmAnd::OfflineMapDataProvider_P::obtainTile( const TileId tileId, const ZoomLevel zoom, std::shared_ptr<const OfflineMapDataTile>& outTile )
{
    // Check if there is a weak reference to that tile, and if that reference is still valid, use that
    std::shared_ptr<TileEntry> tileEntry;
    _tileReferences.obtainOrAllocateEntry(tileEntry, tileId, zoom, [](const TilesCollection<TileEntry>& collection, const TileId tileId, const ZoomLevel zoom) -> TileEntry*
        {
            return new TileEntry(collection, tileId, zoom);
        });

    // Only if tile entry has "Undefined" state proceed to "Loading" state
    if(!tileEntry->setStateIf(TileState::Undefined, TileState::Loading))
    {
        if(tileEntry->getState() == TileState::Loaded)
        {
            // If tile is already 'Loaded', just verify it's reference and return that
            assert(!tileEntry->_tile.expired());
            outTile = tileEntry->_tile.lock();
            return;
        }
        else if(tileEntry->getState() == TileState::Loading)
        {
            QReadLocker scopedLcoker(&tileEntry->_loadedConditionLock);

            // If tile is in 'Loading' state, wait until it will become 'Loaded'
            while(tileEntry->getState() != TileState::Loaded)
                tileEntry->_loadedCondition.wait(&tileEntry->_loadedConditionLock);
        }
    }

#if OSMAND_PERFORMANCE_METRICS
    const auto total_Begin = std::chrono::high_resolution_clock::now();
#endif // OSMAND_PERFORMANCE_METRICS

    // Obtain OBF data interface
#if OSMAND_PERFORMANCE_METRICS
    const auto obtainDataInterface_Begin = std::chrono::high_resolution_clock::now();
#endif // OSMAND_PERFORMANCE_METRICS
    const auto& dataInterface = owner->obfsCollection->obtainDataInterface();
#if OSMAND_PERFORMANCE_METRICS
    const auto obtainDataInterface_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> obtainDataInterface_Elapsed = obtainDataInterface_End - obtainDataInterface_Begin;
#endif // OSMAND_PERFORMANCE_METRICS

    // Get bounding box that covers this tile
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Perform read-out
    QList< std::shared_ptr<const Model::MapObject> > referencedMapObjects;
    QList< proper::shared_future< std::shared_ptr<const Model::MapObject> > > futureReferencedMapObjects;
    QList< std::shared_ptr<const Model::MapObject> > loadedMapObjects;
    QSet< uint64_t > loadedSharedMapObjects;
    MapFoundationType tileFoundation;
#if OSMAND_PERFORMANCE_METRICS
    float dataFilter = 0.0f;
    const auto dataRead_Begin = std::chrono::high_resolution_clock::now();
#endif // OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS > 1
    ObfMapSectionReader_Metrics::Metric_loadMapObjects dataRead_Metric;
#endif // OSMAND_PERFORMANCE_METRICS > 1
    dataInterface->obtainMapObjects(&loadedMapObjects, &tileFoundation, tileBBox31, zoom, nullptr,
        [this, zoom, &referencedMapObjects, &futureReferencedMapObjects, &loadedSharedMapObjects, tileBBox31
#if OSMAND_PERFORMANCE_METRICS
            , &dataFilter
#endif // OSMAND_PERFORMANCE_METRICS
        ](const std::shared_ptr<const ObfMapSectionInfo>& section, const uint64_t id, const AreaI& bbox, const ZoomLevel firstZoomLevel, const ZoomLevel lastZoomLevel) -> bool
        {
#if OSMAND_PERFORMANCE_METRICS
            const auto dataFilter_Begin = std::chrono::high_resolution_clock::now();
#endif // OSMAND_PERFORMANCE_METRICS

            // This map object may be shared only in case it crosses bounds of a tile
            const auto canNotBeShared = tileBBox31.contains(bbox);

            // If map object can not be shared, just read it
            if(canNotBeShared)
            {
#if OSMAND_PERFORMANCE_METRICS
                const auto dataFilter_End = std::chrono::high_resolution_clock::now();
                const std::chrono::duration<float> dataRead_Elapsed = dataFilter_End - dataFilter_Begin;
                dataFilter += dataRead_Elapsed.count();
#endif // OSMAND_PERFORMANCE_METRICS

                return true;
            }
            
            // Otherwise, this map object can be shared, so it should be checked for
            // being present in shared mapObjects storage, or be reserved there
            std::shared_ptr<const Model::MapObject> sharedMapObjectReference;
            proper::shared_future< std::shared_ptr<const Model::MapObject> > futureSharedMapObjectReference;
            if(_sharedMapObjects.obtainReferenceOrFutureReferenceOrMakePromise(id, zoom, Utilities::enumerateZoomLevels(firstZoomLevel, lastZoomLevel), sharedMapObjectReference, futureSharedMapObjectReference))
            {
                if(sharedMapObjectReference)
                {
                    // If map object is already in shared objects cache and is available, use that one
                    referencedMapObjects.push_back(qMove(sharedMapObjectReference));
                }
                else
                {
                    futureReferencedMapObjects.push_back(qMove(futureSharedMapObjectReference));
                }

#if OSMAND_PERFORMANCE_METRICS
                const auto dataFilter_End = std::chrono::high_resolution_clock::now();
                const std::chrono::duration<float> dataRead_Elapsed = dataFilter_End - dataFilter_Begin;
                dataFilter += dataRead_Elapsed.count();
#endif // OSMAND_PERFORMANCE_METRICS
                return false;
            }

            // This map object was reserved, and is going to be shared, but needs to be loaded
            loadedSharedMapObjects.insert(id);
            return true;
        },
#if OSMAND_PERFORMANCE_METRICS > 1
        &dataRead_Metric
#else
        nullptr
#endif // OSMAND_PERFORMANCE_METRICS > 1
    );

#if OSMAND_PERFORMANCE_METRICS
    const auto dataRead_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataRead_Elapsed = dataRead_End - dataRead_Begin;

    const auto dataIdsProcess_Begin = std::chrono::high_resolution_clock::now();
#endif // OSMAND_PERFORMANCE_METRICS

    // Process loaded-and-shared map objects
    for(auto& mapObject : loadedMapObjects)
    {
        // Check if this map object is shared
        if(!loadedSharedMapObjects.contains(mapObject->id))
            continue;

        // Add unique map object under lock to all zoom levels, for which this map object is valid
        assert(mapObject->level);
        _sharedMapObjects.fulfilPromiseAndReference(
            mapObject->id,
            Utilities::enumerateZoomLevels(mapObject->level->minZoom, mapObject->level->maxZoom),
            mapObject);
    }

    for(auto& futureMapObject : futureReferencedMapObjects)
    {
        auto mapObject = futureMapObject.get();

        referencedMapObjects.push_back(qMove(mapObject));
    }

#if OSMAND_PERFORMANCE_METRICS
    const auto dataIdsProcess_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataIdsProcess_Elapsed = dataIdsProcess_End - dataIdsProcess_Begin;

    const auto dataProcess_Begin = std::chrono::high_resolution_clock::now();
#endif // OSMAND_PERFORMANCE_METRICS
#if OSMAND_PERFORMANCE_METRICS > 1
    Rasterizer_Metrics::Metric_prepareContext dataProcess_metric;
#endif // OSMAND_PERFORMANCE_METRICS > 1

    // Prepare data for the tile
    const auto sharedMapObjectsCount = referencedMapObjects.size() + loadedSharedMapObjects.size();
    const auto allMapObjects = loadedMapObjects + referencedMapObjects;

    // Allocate and prepare rasterizer context
    bool nothingToRasterize = false;
    std::shared_ptr<RasterizerContext> rasterizerContext(new RasterizerContext(owner->rasterizerEnvironment, owner->rasterizerSharedContext));
    Rasterizer::prepareContext(*rasterizerContext, tileBBox31, zoom, tileFoundation, allMapObjects, &nothingToRasterize, nullptr,
#if OSMAND_PERFORMANCE_METRICS > 1
        &dataProcess_metric
#else
        nullptr
#endif // OSMAND_PERFORMANCE_METRICS > 1
    );

#if OSMAND_PERFORMANCE_METRICS
    const auto dataProcess_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataProcess_Elapsed = dataProcess_End - dataProcess_Begin;
#endif // OSMAND_PERFORMANCE_METRICS

    // Create tile
    const auto newTile = new OfflineMapDataTile(tileId, zoom, tileFoundation, allMapObjects, rasterizerContext, nothingToRasterize);
    newTile->_d->_link = _link;
    newTile->_d->_refEntry = tileEntry;

    // Publish new tile
    outTile.reset(newTile);

    // Store weak reference to new tile and mark it as 'Loaded'
    tileEntry->_tile = outTile;
    tileEntry->setState(TileState::Loaded);

    // Notify that tile has been loaded
    {
        QWriteLocker scopedLcoker(&tileEntry->_loadedConditionLock);
        tileEntry->_loadedCondition.wakeAll();
    }

#if OSMAND_PERFORMANCE_METRICS
    const auto total_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> total_Elapsed = total_End - total_Begin;
#if OSMAND_PERFORMANCE_METRICS <= 1
    LogPrintf(LogSeverityLevel::Info,
        "%d map objects (%d unique, %d shared) from %dx%d@%d in %fs",
        allMapObjects.size(), allMapObjects.size() - sharedMapObjectsCount, sharedMapObjectsCount,
        tileId.x, tileId.y, zoom,
        total_Elapsed.count());
#else
    LogPrintf(LogSeverityLevel::Info,
        "%d map objects (%d unique, %d shared) from %dx%d@%d in %fs:\n"
        "\topen %fs\n"
        "\tread %fs (filter-by-id %fs):\n"
        "\t - visitedLevels = %d\n"
        "\t - acceptedLevels = %d\n"
        "\t - visitedNodes = %d\n"
        "\t - acceptedNodes = %d\n"
        "\t - elapsedTimeForNodes = %fs\n"
        "\t - mapObjectsBlocksRead = %d\n"
        "\t - visitedMapObjects = %d\n"
        "\t - acceptedMapObjects = %d\n"
        "\t - elapsedTimeForMapObjectsBlocks = %fs\n"
        "\t - elapsedTimeForOnlyVisitedMapObjects = %fs\n"
        "\t - elapsedTimeForOnlyAcceptedMapObjects = %fs\n"
        "\t - average time per 1K only-visited map objects = %fms\n"
        "\t - average time per 1K only-accepted map objects = %fms\n"
        "\tprocess-ids %fs\n"
        "\tprocess-content %fs:\n"
        "\t - elapsedTimeForSortingObjects = %fs\n"
        "\t - elapsedTimeForPolygonizingCoastlines = %fs\n"
        "\t - polygonizedCoastlines = %d\n"
        "\t - elapsedTimeForObtainingPrimitives = %fs\n"
        "\t - elapsedTimeForOrderEvaluation = %fs\n"
        "\t - orderEvaluations = %d\n"
        "\t - average time per 1K order evaluations = %fms\n"
        "\t - elapsedTimeForPolygonEvaluation = %fs\n"
        "\t - polygonEvaluations = %d\n"
        "\t - average time per 1K polygon evaluations = %fms\n"
        "\t - polygonPrimitives = %d\n"
        "\t - elapsedTimeForPolylineEvaluation = %fs\n"
        "\t - polylineEvaluations = %d\n"
        "\t - average time per 1K polyline evaluations = %fms\n"
        "\t - polylinePrimitives = %d\n"
        "\t - elapsedTimeForPointEvaluation = %fs\n"
        "\t - pointEvaluations = %d\n"
        "\t - average time per 1K point evaluations = %fms\n"
        "\t - pointPrimitives = %d\n"
        "\t - elapsedTimeForObtainingPrimitivesSymbols = %fs",
        allMapObjects.size(), allMapObjects.size() - sharedMapObjectsCount, sharedMapObjectsCount,
        tileId.x, tileId.y, zoom,
        total_Elapsed.count(),
        obtainDataInterface_Elapsed.count(),
        dataRead_Elapsed.count(), dataFilter,
        dataRead_Metric.visitedLevels,
        dataRead_Metric.acceptedLevels,
        dataRead_Metric.visitedNodes,
        dataRead_Metric.acceptedNodes,
        dataRead_Metric.elapsedTimeForNodes,
        dataRead_Metric.mapObjectsBlocksRead,
        dataRead_Metric.visitedMapObjects,
        dataRead_Metric.acceptedMapObjects,
        dataRead_Metric.elapsedTimeForMapObjectsBlocks,
        dataRead_Metric.elapsedTimeForOnlyVisitedMapObjects,
        dataRead_Metric.elapsedTimeForOnlyAcceptedMapObjects,
        (dataRead_Metric.elapsedTimeForOnlyVisitedMapObjects * 1000.0f) / (static_cast<float>(dataRead_Metric.visitedMapObjects - dataRead_Metric.acceptedMapObjects) / 1000.0f),
        (dataRead_Metric.elapsedTimeForOnlyAcceptedMapObjects * 1000.0f) / (static_cast<float>(dataRead_Metric.acceptedMapObjects) / 1000.0f),
        dataIdsProcess_Elapsed.count(),
        dataProcess_Elapsed.count(),
        dataProcess_metric.elapsedTimeForSortingObjects,
        dataProcess_metric.elapsedTimeForPolygonizingCoastlines,
        dataProcess_metric.polygonizedCoastlines,
        dataProcess_metric.elapsedTimeForObtainingPrimitives,
        dataProcess_metric.elapsedTimeForOrderEvaluation,
        dataProcess_metric.orderEvaluations,
        (dataProcess_metric.elapsedTimeForOrderEvaluation * 1000.0f / static_cast<float>(dataProcess_metric.orderEvaluations)) * 1000.0f,
        dataProcess_metric.elapsedTimeForPolygonEvaluation,
        dataProcess_metric.polygonEvaluations,
        (dataProcess_metric.elapsedTimeForPolygonEvaluation * 1000.0f / static_cast<float>(dataProcess_metric.polygonEvaluations)) * 1000.0f,
        dataProcess_metric.polygonPrimitives,
        dataProcess_metric.elapsedTimeForPolylineEvaluation,
        dataProcess_metric.polylineEvaluations,
        (dataProcess_metric.elapsedTimeForPolylineEvaluation * 1000.0f / static_cast<float>(dataProcess_metric.polylineEvaluations)) * 1000.0f,
        dataProcess_metric.polylinePrimitives,
        dataProcess_metric.elapsedTimeForPointEvaluation,
        dataProcess_metric.pointEvaluations,
        (dataProcess_metric.elapsedTimeForPointEvaluation * 1000.0f / static_cast<float>(dataProcess_metric.pointEvaluations)) * 1000.0f,
        dataProcess_metric.pointPrimitives,
        dataProcess_metric.elapsedTimeForObtainingPrimitivesSymbols);
#endif // OSMAND_PERFORMANCE_METRICS <= 1
#endif // OSMAND_PERFORMANCE_METRICS
}

