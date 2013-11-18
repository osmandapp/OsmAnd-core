#include "OfflineMapDataProvider_P.h"
#include "OfflineMapDataProvider.h"

#include <cassert>
#if defined(_DEBUG) || defined(DEBUG)
#   include <chrono>
#endif

#include "OfflineMapDataTile.h"
#include "OfflineMapDataTile_P.h"
#include "ObfsCollection.h"
#include "ObfDataInterface.h"
#include "ObfMapSectionInfo.h"
#if defined(_DEBUG) || defined(DEBUG)
#   include "ObfMapSectionReader_Metrics.h"
#   include "Rasterizer_Metrics.h"
#endif
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

#if defined(_DEBUG) || defined(DEBUG)
    const auto total_Begin = std::chrono::high_resolution_clock::now();
#endif

    // Obtain OBF data interface
#if defined(_DEBUG) || defined(DEBUG)
    const auto obtainDataInterface_Begin = std::chrono::high_resolution_clock::now();
#endif
    const auto& dataInterface = owner->obfsCollection->obtainDataInterface();
#if defined(_DEBUG) || defined(DEBUG)
    const auto obtainDataInterface_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> obtainDataInterface_Elapsed = obtainDataInterface_End - obtainDataInterface_Begin;
#endif

    // Get bounding box that covers this tile
    const auto tileBBox31 = Utilities::tileBoundingBox31(tileId, zoom);

    // Perform read-out
    QList< std::shared_ptr<const Model::MapObject> > sharedMapObjects;
    QList< std::shared_ptr<const Model::MapObject> > mapObjects;
    MapFoundationType tileFoundation;
#if defined(_DEBUG) || defined(DEBUG)
    float dataFilter = 0.0f;
    const auto dataRead_Begin = std::chrono::high_resolution_clock::now();
    ObfMapSectionReader_Metrics::Metric_loadMapObjects dataRead_Metric;
#endif
    auto& dataCache = _dataCache[zoom];
    dataInterface->obtainMapObjects(&mapObjects, &tileFoundation, tileBBox31, zoom, nullptr,
        /*
#if defined(_DEBUG) || defined(DEBUG)
        [&dataCache, &sharedMapObjects, tileBBox31, &dataFilter](const std::shared_ptr<const ObfMapSectionInfo>& section, const uint64_t id) -> bool
#else
        [&dataCache, &sharedMapObjects, tileBBox31](const std::shared_ptr<const ObfMapSectionInfo>& section, const uint64_t id) -> bool
#endif
        {
#if defined(_DEBUG) || defined(DEBUG)
            const auto dataFilter_Begin = std::chrono::high_resolution_clock::now();
#endif

            // Save reference to duplicate map object
            {
                QReadLocker scopedLocker(&dataCache._mapObjectsMutex);

                const auto itSharedMapObject = dataCache._mapObjects.constFind(id);
                if(itSharedMapObject != dataCache._mapObjects.cend())
                {
                    const auto& mapObjectWeakRef = *itSharedMapObject;

                    if(const auto& mapObject = mapObjectWeakRef.lock())
                    {
                        // Not all duplicates should be used, since some may lay outside bbox
                        if(mapObject->intersects(tileBBox31))
                            sharedMapObjects.push_back(mapObject);

#if defined(_DEBUG) || defined(DEBUG)
                        const auto dataFilter_End = std::chrono::high_resolution_clock::now();
                        const std::chrono::duration<float> dataRead_Elapsed = dataFilter_End - dataFilter_Begin;
                        dataFilter += dataRead_Elapsed.count();
#endif

                        return false;
                    }
                }
            }

#if defined(_DEBUG) || defined(DEBUG)
            const auto dataFilter_End = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<float> dataRead_Elapsed = dataFilter_End - dataFilter_Begin;
            dataFilter += dataRead_Elapsed.count();
#endif

            return true;
        },*/nullptr,
#if defined(_DEBUG) || defined(DEBUG)
        &dataRead_Metric
#else
        nullptr
#endif
    );

#if defined(_DEBUG) || defined(DEBUG)
    const auto dataRead_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataRead_Elapsed = dataRead_End - dataRead_Begin;

    const auto dataIdsProcess_Begin = std::chrono::high_resolution_clock::now();
#endif
    /*
    // Append weak references to newly read map objects
    for(auto itMapObject = mapObjects.cbegin(); itMapObject != mapObjects.cend(); ++itMapObject)
    {
        const auto& mapObject = *itMapObject;

        // Add unique map object under lock to all zoom levels, for which this map object is valid
        assert(mapObject->level);
        for(int zoomLevel = mapObject->level->minZoom; zoomLevel <= mapObject->level->maxZoom; zoomLevel++)
        {
            auto& dataCache = _dataCache[zoomLevel];
            {
                QWriteLocker scopedLocker(&dataCache._mapObjectsMutex);

                if(!dataCache._mapObjects.contains(mapObject->id))
                    dataCache._mapObjects.insert(mapObject->id, mapObject);
            }
        }
    }
    */
#if defined(_DEBUG) || defined(DEBUG)
    const auto dataIdsProcess_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataIdsProcess_Elapsed = dataIdsProcess_End - dataIdsProcess_Begin;

    Rasterizer_Metrics::Metric_prepareContext dataProcess_metric;
    const auto dataProcess_Begin = std::chrono::high_resolution_clock::now();
#endif

    // Prepare data for the tile
    mapObjects << sharedMapObjects;

    // Allocate and prepare rasterizer context
    bool nothingToRasterize = false;
    std::shared_ptr<RasterizerContext> rasterizerContext(new RasterizerContext(owner->rasterizerEnvironment));
    Rasterizer::prepareContext(*rasterizerContext, tileBBox31, zoom, tileFoundation, mapObjects, &nothingToRasterize, nullptr,
#if defined(_DEBUG) || defined(DEBUG)
        &dataProcess_metric
#else
#endif
    );

#if defined(_DEBUG) || defined(DEBUG)
    const auto dataProcess_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> dataProcess_Elapsed = dataProcess_End - dataProcess_Begin;
#endif

    // Create tile
    const auto newTile = new OfflineMapDataTile(tileId, zoom, tileFoundation, mapObjects, rasterizerContext, nothingToRasterize);
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

#if defined(_DEBUG) || defined(DEBUG)
    const auto total_End = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<float> total_Elapsed = total_End - total_Begin;

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
        "\t - elapsedTimeForCombiningObjects = %fs\n"
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
        mapObjects.size(), mapObjects.size() - sharedMapObjects.size(), sharedMapObjects.size(),
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
        dataProcess_metric.elapsedTimeForCombiningObjects,
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
#endif
}

