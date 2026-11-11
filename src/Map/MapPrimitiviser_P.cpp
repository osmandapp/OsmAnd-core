#include "MapPrimitiviser_P.h"
#include <limits>
#include "MapPrimitiviser.h"

#include "stdlib_common.h"
#include <proper/future.h>
#include <set>
#include <algorithm>
#include <memory>

#include "QtExtensions.h"
#include "QtCommon.h"

#include "Nullable.h"
#include "ICU.h"
#include "MapStyleEvaluator.h"
#include "MapStyleEvaluationResult.h"
#include "MapStyleBuiltinValueDefinitions.h"
#include "ObfMapSectionInfo.h"
#include "MapObject.h"
#include "BinaryMapObject.h"
#include "Road.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "QKeyValueIterator.h"
#include "QCachingIterator.h"
#include "Logging.h"
#include "multipolygons.h"

#include <string>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace
{
    bool zbFlagB()
    {
        static const bool enabled = []() -> bool
        {
            char buffer[96] = { 0 };
#if defined(__ANDROID__)
            __system_property_get("debug.osmand.zb", buffer);
#endif
            return std::string(buffer).find('B') != std::string::npos;
        }();
        return enabled;
    }

    struct ZbEvalKey
    {
        quint64 signature;
        quint32 tag;
        quint32 value;
        int ruleset;
        const void* env;
        int zoom;
        bool operator==(const ZbEvalKey& o) const
        {
            return signature == o.signature && tag == o.tag && value == o.value && ruleset == o.ruleset
                && env == o.env && zoom == o.zoom;
        }
    };
    inline uint qHash(const ZbEvalKey& k, uint seed = 0)
    {
        quint64 h = k.signature * 0x9E3779B97F4A7C15ull;
        h ^= (static_cast<quint64>(k.tag) << 32) | k.value;
        h ^= static_cast<quint64>(k.ruleset) << 56 ^ static_cast<quint64>(k.zoom) << 48;
        h ^= reinterpret_cast<quintptr>(k.env);
        return ::qHash(h, seed);
    }
    struct ZbEvalValue
    {
        bool ok;
        OsmAnd::MapStyleEvaluationResult::Packed packed;
    };
    QHash<ZbEvalKey, ZbEvalValue>& zbEvalMemo()
    {
        static thread_local QHash<ZbEvalKey, ZbEvalValue> memo;
        return memo;
    }

    inline void zbMix(quint64& h, const quint64 v)
    {
        h ^= v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
    }
    quint64 zbObjectSignature(const std::shared_ptr<const OsmAnd::MapObject>& o)
    {
        quint64 h = reinterpret_cast<quintptr>(o->attributeMapping.get());
        for (const auto id : OsmAnd::constOf(o->attributeIds))
            zbMix(h, id);
        zbMix(h, 0xFFFFFFFFull);
        for (const auto id : OsmAnd::constOf(o->additionalAttributeIds))
            zbMix(h, id);
        zbMix(h, static_cast<quint64>(o->getLayerType()) + 7);
        zbMix(h, (o->isArea ? 1 : 0) | (o->points31.size() == 1 ? 2 : 0) | (o->isClosedFigure() ? 4 : 0));
        for (auto it = o->captions.cbegin(); it != o->captions.cend(); ++it)
        {
            zbMix(h, it.key());
            zbMix(h, static_cast<quint64>(it.value().length()));
        }
        return h;
    }

    bool zbEvaluate(
        const OsmAnd::MapStyleEvaluator& evaluator,
        const std::shared_ptr<const OsmAnd::MapObject>& mapObject,
        const OsmAnd::MapStyleRulesetType ruleset,
        OsmAnd::MapStyleEvaluationResult& result,
        const quint64 signature,
        const quint32 tag,
        const quint32 value,
        const void* env,
        const int zoom)
    {
        if (!zbFlagB())
            return evaluator.evaluate(mapObject, ruleset, &result);
        auto& memo = zbEvalMemo();
        const ZbEvalKey key{ signature, tag, value, static_cast<int>(ruleset), env, zoom };
        const auto it = memo.constFind(key);
        if (it != memo.cend())
        {
            result.clear();
            for (const auto& entry : OsmAnd::constOf(it->packed.entries))
                result.setValue(entry.first, entry.second);
            return it->ok;
        }
        const bool ok = evaluator.evaluate(mapObject, ruleset, &result);
        if (memo.size() > 200000)
            memo.clear();
        ZbEvalValue v;
        v.ok = ok;
        result.pack(v.packed);
        memo.insert(key, v);
        return ok;
    }

    // parseValue memo per decoded attribute address (decodeMap entries live as long as the section)
    QHash<QPair<const void*, const void*>, QPair<quint32, quint32>>& zbParsedTagValues()
    {
        static thread_local QHash<QPair<const void*, const void*>, QPair<quint32, quint32>> memo;
        return memo;
    }
}


#define USE_V1_COASTLINES 0

//#define OSMAND_VERBOSE_MAP_PRIMITIVISER 1
#if !defined(OSMAND_VERBOSE_MAP_PRIMITIVISER)
#   define OSMAND_VERBOSE_MAP_PRIMITIVISER 0
#endif // !defined(OSMAND_VERBOSE_MAP_PRIMITIVISER)

#define DEFAULT_TEXT_MIN_DISTANCE 150

// If object's primitive is polygon and order is > 10, add it to polylines list
// If object's primitive is polyline and order < 11, add it to polygons list
#define MAX_POLYGON_ORDER_IN_POLYGONS_LIST 10
#define MIN_POLYLINE_ORDER_IN_POLYLINES_LIST (MAX_POLYGON_ORDER_IN_POLYGONS_LIST + 1)

namespace OsmAnd
{
    inline uint qHash(const PointI& p, uint seed = 0) Q_DECL_NOTHROW
    {
        const auto key = static_cast<quint64>(static_cast<quint32>(p.x))
            | (static_cast<quint64>(static_cast<quint32>(p.y)) << 32);
        return ::qHash(key, seed);
    }
}

OsmAnd::MapPrimitiviser_P::MapPrimitiviser_P(MapPrimitiviser* const owner_)
    : owner(owner_)
{
}

OsmAnd::MapPrimitiviser_P::~MapPrimitiviser_P()
{
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseAllMapObjects(
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric)
{
    return primitiviseAllMapObjects(
        PointD(-1.0, -1.0),
        zoom,
        TileId::fromXY(-1, -1),
        objects,
        cache,
        queryController,
        metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseAllMapObjects(
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric)
{
    return primitiviseAllMapObjects(
        PointD(-1.0, -1.0),
        zoom,
        tileId,
        objects,
        cache,
        queryController,
        metric);
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseAllMapObjects(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseAllMapObjects* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    const Context context(owner->environment, zoom);
    const std::shared_ptr<PrimitivisedObjects> primitivisedObjects(new PrimitivisedObjects(
        owner->environment,
        cache,
        zoom,
        scaleDivisor31ToPixel));

    // Check if there are map objects for primitivisation
    if (objects.isEmpty())
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult(context.env->mapStyle->getValueDefinitionsCount());

    const Stopwatch obtainPrimitivesStopwatch(metric != nullptr);
    obtainPrimitives(context, zoom, primitivisedObjects, objects, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitives += obtainPrimitivesStopwatch.elapsed();
    
    if (queryController && queryController->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects, metric);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, tileId, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseWithSurface(
    const AreaI area31,
    const PointI areaSizeInPixels,
    const ZoomLevel zoom,
    const ZoomLevel detailedZoom,
    const TileId tileId,
    const AreaI visibleArea31,
    const int64_t visibleAreaTime,
    const MapSurfaceType surfaceType_,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseWithSurface* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);
    const Context context(owner->environment, zoom);
    const std::shared_ptr<PrimitivisedObjects> primitivisedObjects(new PrimitivisedObjects(
        owner->environment,
        cache,
        zoom,
        Utilities::getScaleDivisor31ToPixel(areaSizeInPixels, zoom)));

    const Stopwatch objectsSortingStopwatch(metric != nullptr);

    // Split input map objects to object, coastline, basemapObjects and basemapCoastline
    QList< std::shared_ptr<const MapObject> > detailedmapMapObjects;
    QList< std::shared_ptr<const MapObject> > detailedmapCoastlineObjects;
    QList< std::shared_ptr<const MapObject> > basemapMapObjects;
    QList< std::shared_ptr<const MapObject> > basemapCoastlineObjects;
    QList< std::shared_ptr<const MapObject> > extraCoastlineObjects;
    int contourLinesObjectsCount = 0;

    const auto enlargedArea31 = Utilities::getEnlargedPrimitivesArea(area31);

    for (const auto& mapObject : constOf(objects))
    {
        if (queryController && queryController->isAborted())
            break;

        // Check overscaled coastline objects of 13 zoom
        if (mapObject->isCoastline)
        {
            const auto binaryMapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject);
            if (binaryMapObject)
            {
                extraCoastlineObjects.push_back(mapObject);
            }
            continue;
        }
        
        bool isInArea = false;
        if (mapObject->needsSimplification(visibleArea31))
        {
            QVector<PointI> path31;
            path31.reserve(mapObject->points31.size());
            isInArea = mapObject->intersectedOrContainedBy(enlargedArea31, visibleArea31, visibleAreaTime, &path31);
            if (isInArea || !path31.isEmpty())
                mapObject->updateVisibleArea(visibleArea31, visibleAreaTime, path31.isEmpty() ? nullptr : &path31);
        }
        else
            isInArea = mapObject->intersectedOrContainedBy(enlargedArea31, visibleArea31, visibleAreaTime, nullptr);

        if(!isInArea && !mapObject->containsAttribute(mapObject->attributeMapping->naturalCoastlineAttributeId))
        {
            continue;
        }

        // Check origin of map object
        auto isBasemapObject = false;
        auto isContourLinesObject = false;
        if (const auto binaryMapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject))
        {
            isBasemapObject = binaryMapObject->section->isBasemap;
            isContourLinesObject = binaryMapObject->section->isContourLines;
        }

        if (mapObject->containsAttribute(mapObject->attributeMapping->naturalCoastlineAttributeId))
        {
            if (isBasemapObject)
                basemapCoastlineObjects.push_back(mapObject);
            else
                detailedmapCoastlineObjects.push_back(mapObject);
        }
        else
        {
            if (isBasemapObject)
                basemapMapObjects.push_back(mapObject);
            else
            {
                detailedmapMapObjects.push_back(mapObject);
                if (isContourLinesObject)
                    contourLinesObjectsCount++;
            }
        }
    }
    
    bool hasContourLinesObjectOnly = contourLinesObjectsCount == detailedmapMapObjects.size();
    
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTimeForSortingObjects += objectsSortingStopwatch.elapsed();

    const Stopwatch polygonizeCoastlinesStopwatch(metric != nullptr);

    // Polygonize coastlines
    bool coastlineMistake = false;
    QList< std::shared_ptr<const MapObject> > polygonizedCoastlineObjects;
    const auto basemapCoastlinesPresent = !basemapCoastlineObjects.isEmpty();
    const auto detailedmapCoastlinesPresent = !detailedmapCoastlineObjects.isEmpty();
    auto surfaceType = surfaceType_;
    auto fillEntireArea = true;
    if (USE_V1_COASTLINES)
    {
        bool detailCoastlineBroken = false;
        
        if (detailedmapCoastlinesPresent && zoom >= MapPrimitiviser::DetailedLandDataMinZoom)
        {
            const bool coastlinesWereAdded = polygonizeCoastlines(
                getWidenArea(area31, zoom),
                zoom,
                detailedmapCoastlineObjects,
                polygonizedCoastlineObjects);
            fillEntireArea = !coastlinesWereAdded;
            
            if (!coastlinesWereAdded)
            {
                //detect if broken coastline inside area31 (tile bbox)
                for (auto obj : detailedmapCoastlineObjects)
                {
                    for (auto point : obj->points31)
                    {
                        if (area31.contains(point))
                        {
                            detailCoastlineBroken = true;
                            break;
                        }
                    }
                    if (detailCoastlineBroken)
                        break;
                }
            }
        }
        
        
        bool hasExtraCoastlines = !extraCoastlineObjects.isEmpty();
        bool shouldAddBasemapCoastlines = !detailedmapCoastlinesPresent
                                    && !hasExtraCoastlines
                                    //&& !detailedLandDataPresent
                                    && basemapCoastlinesPresent;
        
        if (detailCoastlineBroken && basemapCoastlinesPresent)
        {
            shouldAddBasemapCoastlines = true;
            polygonizedCoastlineObjects.clear();
        }
        
        shouldAddBasemapCoastlines = shouldAddBasemapCoastlines || zoom < MapPrimitiviser::DetailedLandDataMinZoom;
        
        if (shouldAddBasemapCoastlines)
        {
            const bool coastlinesWereAdded = polygonizeCoastlines(
                area31,
                zoom,
                basemapCoastlineObjects,
                polygonizedCoastlineObjects);
            fillEntireArea = !coastlinesWereAdded && fillEntireArea;
        }

        // In case zoom is higher than ObfMapSectionLevel::MaxBasemapZoomLevel and coastlines were not used
        // due to none of them intersect current zoom tile edge, look for the nearest coastline segment
        // to determine use FullLand or FullWater as surface type
        if (zoom > ObfMapSectionLevel::MaxBasemapZoomLevel && fillEntireArea)
        {
            const auto center = area31.center();
            assert(area31.contains(center));
            bool isDeterminedSurfaceType = false;
            
            if (hasExtraCoastlines)
            {
                AreaI bboxZoom11 = Utilities::roundBoundingBox31(area31, ZoomLevel::ZoomLevel12);
                bboxZoom11.right()++;
                bboxZoom11.bottom()++;
                bboxZoom11 = bboxZoom11.getEnlargedBy(bboxZoom11.width() / 2);
                bboxZoom11.right()--;
                bboxZoom11.bottom()--;
                MapSurfaceType surfaceTypeOverscaled = MapSurfaceType::Undefined;
                QList< std::shared_ptr<const MapObject> > polygonizedCoastlines;
                polygonizeCoastlines(
                    bboxZoom11,
                    ZoomLevel::ZoomLevel11,
                    extraCoastlineObjects,
                    polygonizedCoastlines);
                surfaceTypeOverscaled = determineSurfaceType(area31, polygonizedCoastlines);
                if (surfaceTypeOverscaled != MapSurfaceType::Undefined)
                {
                    isDeterminedSurfaceType = true;
                    surfaceType = surfaceTypeOverscaled;
                }
            }
            
            if (!isDeterminedSurfaceType && basemapCoastlinesPresent)
            {
                ZoomLevel basemapZoom = static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel);
                AreaI bboxBasemap = Utilities::roundBoundingBox31(area31, basemapZoom);
                QList< std::shared_ptr<const MapObject> > polygonizedCoastlines;
                MapSurfaceType surfaceTypeBasemap = MapSurfaceType::Undefined;
                polygonizeCoastlines(
                    bboxBasemap,
                    basemapZoom,
                    basemapCoastlineObjects,
                    polygonizedCoastlines);
                surfaceTypeBasemap = determineSurfaceType(area31, polygonizedCoastlines);
                if (surfaceTypeBasemap != MapSurfaceType::Undefined)
                {
                    isDeterminedSurfaceType = true;
                    surfaceType = surfaceTypeBasemap;
                }
            }
        }
    }
    else
    {
        // As core-legacy useDetailedCoastlines: a tile at latitude lat is 1/cos(lat) times smaller on the ground,
        // so above ~60 degrees the basemap coastline is too coarse one zoom earlier
        const double centerLat = Utilities::get31LatitudeY(area31.top() / 2 + area31.bottom() / 2);
        const double groundZoom = zoom + std::log2(1.0 / qMax(0.01, std::cos(centerLat * M_PI / 180.0)));
        const bool useDetailedLandData = zoom >= MapPrimitiviser::DetailedLandDataMinZoom
            || groundZoom > MapPrimitiviser::DetailedLandDataMinZoom;

        auto coastlinesWereAdded = false;
        bool withBrokenCoastline = false;
        if (detailedmapCoastlinesPresent && useDetailedLandData)
        {
            coastlinesWereAdded = getCoastlines(area31, AreaI64(area31),
                detailedmapCoastlineObjects, polygonizedCoastlineObjects, surfaceType, &withBrokenCoastline);
        }
        bool hasExtraCoastlines = !withBrokenCoastline && !extraCoastlineObjects.isEmpty();
        if (!coastlinesWereAdded && hasExtraCoastlines && zoom > ObfMapSectionLevel::MaxBasemapZoomLevel)
        {
            auto bboxZoom11wide = AreaI64(Utilities::roundBoundingBox31(area31, ZoomLevel::ZoomLevel11));
            bboxZoom11wide.right()++;
            bboxZoom11wide.bottom()++;
            bboxZoom11wide = bboxZoom11wide.getEnlargedBy(bboxZoom11wide.width() / 2);
            bboxZoom11wide.right()--;
            bboxZoom11wide.bottom()--;
            QList< std::shared_ptr<const MapObject> > polygonizedCoastlines;
            auto extraSurfaceType = MapSurfaceType::Undefined;
            getCoastlines(area31, bboxZoom11wide,
                extraCoastlineObjects, polygonizedCoastlines, extraSurfaceType, &withBrokenCoastline);
            if (extraSurfaceType == MapSurfaceType::Undefined)
                hasExtraCoastlines = false;
            else
                surfaceType = extraSurfaceType;
        }
        if (!coastlinesWereAdded && basemapCoastlinesPresent
            && (!hasExtraCoastlines || !useDetailedLandData))
        {
            const auto baseZoom = static_cast<ZoomLevel>(ObfMapSectionLevel::MaxBasemapZoomLevel);
            AreaI64 basemapArea(area31);
            if (zoom > baseZoom)
            {
                basemapArea = AreaI64(Utilities::roundBoundingBox31(area31, baseZoom));
                basemapArea.right()++;
                basemapArea.bottom()++;
                basemapArea = basemapArea.getEnlargedBy(1ll << (ZoomLevel31 - baseZoom));
            }
            coastlinesWereAdded = getCoastlines(area31, basemapArea,
                basemapCoastlineObjects, polygonizedCoastlineObjects, surfaceType);
            coastlineMistake = coastlinesWereAdded && !withBrokenCoastline
                && zoom > ObfMapSectionLevel::MaxBasemapZoomLevel;
        }
        fillEntireArea = !coastlinesWereAdded;
    }

    primitivisedObjects->coastlineMistake = coastlineMistake;

    if (metric)
    {
        metric->elapsedTimeForPolygonizingCoastlines += polygonizeCoastlinesStopwatch.elapsed();
        metric->polygonizedCoastlines += polygonizedCoastlineObjects.size();
    }

    if (basemapMapObjects.isEmpty() && detailedmapMapObjects.isEmpty() && surfaceType == MapSurfaceType::Undefined)
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    if (fillEntireArea)
    {
        const std::shared_ptr<MapObject> bgMapObject(new SurfaceMapObject());
        bgMapObject->isArea = true;
        bgMapObject->points31.push_back(qMove(PointI(area31.left(), area31.top())));
        bgMapObject->points31.push_back(qMove(PointI(area31.right(), area31.top())));
        bgMapObject->points31.push_back(qMove(PointI(area31.right(), area31.bottom())));
        bgMapObject->points31.push_back(qMove(PointI(area31.left(), area31.bottom())));
        bgMapObject->points31.push_back(bgMapObject->points31.first());
        bgMapObject->bbox31 = area31;
        if (surfaceType == MapSurfaceType::FullWater)
            bgMapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineAttributeId);
        else if (surfaceType == MapSurfaceType::FullLand || surfaceType == MapSurfaceType::Mixed)
            bgMapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalLandAttributeId);
        else // if (surfaceType == SurfaceType::Undefined)
        {
#if OSMAND_VERBOSE_MAP_PRIMITIVISER
            LogPrintf(LogSeverityLevel::Warning, "Area [%d, %d, %d, %d]@%d has undefined surface type",
                area31.top(),
                area31.left(),
                area31.bottom(),
                area31.right(),
                zoom);
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER

            bgMapObject->isArea = false;
            bgMapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineBrokenAttributeId);
        }
        bgMapObject->additionalAttributeIds.push_back(MapObject::defaultAttributeMapping->layerLowestAttributeId);

        assert(bgMapObject->isClosedFigure());
        if (bgMapObject->isArea)
            polygonizedCoastlineObjects.push_back(qMove(bgMapObject));
    }

    // Obtain primitives
    const bool detailedDataMissing =
        (zoom > static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) &&
        (detailedmapMapObjects.isEmpty() || hasContourLinesObjectOnly) &&
        detailedmapCoastlineObjects.isEmpty();

    // Check if there is no data to primitivise. Report, clean-up and exit
    const auto mapObjectsCount =
        detailedmapMapObjects.size() +
        ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) || detailedDataMissing)
            ? basemapMapObjects.size()
            : 0) +
        polygonizedCoastlineObjects.size();
    if (mapObjectsCount == 0)
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult(context.env->mapStyle->getValueDefinitionsCount());

    const Stopwatch obtainPrimitivesFromDetailedmapStopwatch(metric != nullptr);
    obtainPrimitives(
        context,
        detailedZoom,
        primitivisedObjects,
        detailedmapMapObjects,
        evaluationResult,
        cache,
        queryController,
        metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromDetailedmap += obtainPrimitivesFromDetailedmapStopwatch.elapsed();

    if ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) || detailedDataMissing)
    {
        const Stopwatch obtainPrimitivesFromBasemapStopwatch(metric != nullptr);
        obtainPrimitives(
            context,
            detailedZoom,
            primitivisedObjects,
            basemapMapObjects,
            evaluationResult,
            cache,
            queryController,
            metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesFromBasemap += obtainPrimitivesFromBasemapStopwatch.elapsed();
    }

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
    debugCoastline(area31, polygonizedCoastlineObjects);
#endif
    
    const Stopwatch obtainPrimitivesFromCoastlinesStopwatch(metric != nullptr);
    obtainPrimitives(
        context,
        detailedZoom,
        primitivisedObjects,
        polygonizedCoastlineObjects,
        evaluationResult,
        cache,
        queryController,
        metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromCoastlines += obtainPrimitivesFromCoastlinesStopwatch.elapsed();

    if (queryController && queryController->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects, metric);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, tileId, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

std::shared_ptr<OsmAnd::MapPrimitiviser_P::PrimitivisedObjects> OsmAnd::MapPrimitiviser_P::primitiviseWithoutSurface(
    const PointD scaleDivisor31ToPixel,
    const ZoomLevel zoom,
    const TileId tileId,
    const QList< std::shared_ptr<const MapObject> >& objects,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitiviseWithoutSurface* const metric)
{
    const Stopwatch totalStopwatch(metric != nullptr);

    const Context context(owner->environment, zoom);
    const std::shared_ptr<PrimitivisedObjects> primitivisedObjects(new PrimitivisedObjects(
        owner->environment,
        cache, 
        zoom,
        scaleDivisor31ToPixel));

    const Stopwatch objectsSortingStopwatch(metric != nullptr);

    // Split input map objects
    QList< std::shared_ptr<const MapObject> > detailedmapMapObjects, basemapMapObjects;
    for (const auto& mapObject : constOf(objects))
    {
        if (queryController && queryController->isAborted())
            break;

        // Check if this map object is from basemap
        auto isBasemapObject = false;
        if (const auto possiblyBasemapObject = std::dynamic_pointer_cast<const BinaryMapObject>(mapObject))
        {
            isBasemapObject = possiblyBasemapObject->section->isBasemap;
            if (zoom < static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) && !isBasemapObject)
                continue;
        }

        if (!mapObject->containsAttribute(mapObject->attributeMapping->naturalCoastlineAttributeId))
        {
            if (isBasemapObject)
                basemapMapObjects.push_back(mapObject);
            else
                detailedmapMapObjects.push_back(mapObject);
        }
    }
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTimeForSortingObjects += objectsSortingStopwatch.elapsed();

    // Obtain primitives
    const bool detailedDataMissing = (zoom > static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) && detailedmapMapObjects.isEmpty();

    // Check if there is no data to primitivise. Report, clean-up and exit
    const auto mapObjectsCount =
        detailedmapMapObjects.size() +
        ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap) || detailedDataMissing) ? basemapMapObjects.size() : 0);
    if (mapObjectsCount == 0)
    {
        // Empty area
        assert(primitivisedObjects->isEmpty());
        return primitivisedObjects;
    }

    // Obtain primitives:
    MapStyleEvaluationResult evaluationResult(context.env->mapStyle->getValueDefinitionsCount());

    const Stopwatch obtainPrimitivesFromDetailedmapStopwatch(metric != nullptr);
    obtainPrimitives(
        context, zoom, primitivisedObjects, detailedmapMapObjects, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesFromDetailedmap += obtainPrimitivesFromDetailedmapStopwatch.elapsed();

    if ((zoom <= static_cast<ZoomLevel>(MapPrimitiviser::LastZoomToUseBasemap)) || detailedDataMissing)
    {
        const Stopwatch obtainPrimitivesFromBasemapStopwatch(metric != nullptr);
        obtainPrimitives(
            context, zoom, primitivisedObjects, basemapMapObjects, evaluationResult, cache, queryController, metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesFromBasemap += obtainPrimitivesFromBasemapStopwatch.elapsed();
    }

    if (queryController && queryController->isAborted())
        return nullptr;

    // Sort and filter primitives
    const Stopwatch sortAndFilterPrimitivesStopwatch(metric != nullptr);
    sortAndFilterPrimitives(context, primitivisedObjects, metric);
    if (metric)
        metric->elapsedTimeForSortingAndFilteringPrimitives += sortAndFilterPrimitivesStopwatch.elapsed();

    // Obtain symbols from primitives
    const Stopwatch obtainPrimitivesSymbolsStopwatch(metric != nullptr);
    obtainPrimitivesSymbols(context, primitivisedObjects, tileId, evaluationResult, cache, queryController, metric);
    if (metric)
        metric->elapsedTimeForObtainingPrimitivesSymbols += obtainPrimitivesSymbolsStopwatch.elapsed();

    // Cleanup if aborted
    if (queryController && queryController->isAborted())
        return nullptr;

    if (metric)
        metric->elapsedTime += totalStopwatch.elapsed();

    return primitivisedObjects;
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitives(
    const Context& context,
    const ZoomLevel detailedZoom,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const QList< std::shared_ptr<const OsmAnd::MapObject> >& source,
    MapStyleEvaluationResult& evaluationResult,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;
    const auto zoom = primitivisedObjects->zoom;
    auto detailScaleFactor = static_cast<float>(1 << abs(detailedZoom - zoom));
    if (detailedZoom > zoom)
        detailScaleFactor = 1.0f / detailScaleFactor;

    const Stopwatch obtainPrimitivesStopwatch(metric != nullptr);

    // Initialize shared settings for order evaluation
    MapStyleEvaluator orderEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(orderEvaluator);
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for polygon evaluation
    MapStyleEvaluator polygonEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(polygonEvaluator);
    polygonEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    polygonEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Initialize shared settings for polyline evaluation
    MapStyleEvaluator polylineEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(polylineEvaluator);
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, detailedZoom);
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, detailedZoom);

    // Initialize shared settings for point evaluation
    MapStyleEvaluator pointEvaluator(env->mapStyle, env->displayDensityFactor * env->mapScaleFactor);
    env->applyTo(pointEvaluator);
    pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    const auto pSharedPrimitivesGroups = cache ? cache->getPrimitivesGroupsPtr(zoom) : nullptr;
    QList< proper::shared_future< std::shared_ptr<const PrimitivesGroup> > > futureSharedPrimitivesGroups;
    for (const auto& mapObject : constOf(source))
    {
        //////////////////////////////////////////////////////////////////////////
        //if (mapObject->toString().contains("1333827773"))
        //{
        //    const auto t = mapObject->toString();
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        if (queryController && queryController->isAborted())
            return;

        MapObject::SharingKey sharingKey;
        const auto isShareable = mapObject->obtainSharingKey(sharingKey);

        // If group can be shared, use already-processed or reserve pending
        if (pSharedPrimitivesGroups && isShareable)
        {
            // If this group was already processed, use that
            std::shared_ptr<const PrimitivesGroup> group;
            proper::shared_future< std::shared_ptr<const PrimitivesGroup> > futureGroup;
            if (pSharedPrimitivesGroups->obtainReferenceOrFutureReferenceOrMakePromise(sharingKey, group, futureGroup))
            {
                if (group)
                {
                    // Add polygons, polylines and points from group to current context
                    primitivisedObjects->polygons.append(group->polygons);
                    primitivisedObjects->polylines.append(group->polylines);
                    primitivisedObjects->points.append(group->points);

                    // Add shared group to current context
                    primitivisedObjects->primitivesGroups.push_back(qMove(group));
                }
                else
                {
                    futureSharedPrimitivesGroups.push_back(qMove(futureGroup));
                }

                continue;
            }
        }

        // Create a primitives group
        const Stopwatch obtainPrimitivesGroupStopwatch(metric != nullptr);
        const auto group = obtainPrimitivesGroup(
            context,
            detailScaleFactor,
            primitivisedObjects,
            mapObject,
            evaluationResult,
            orderEvaluator,
            polygonEvaluator,
            polylineEvaluator,
            pointEvaluator,
            metric);
        if (metric)
            metric->elapsedTimeForObtainingPrimitivesGroups += obtainPrimitivesGroupStopwatch.elapsed();

        // Add this group to shared cache
        if (pSharedPrimitivesGroups && isShareable)
            pSharedPrimitivesGroups->fulfilPromiseAndReference(sharingKey, group);

        // Add polygons, polylines and points from group to current context
        primitivisedObjects->polygons.append(group->polygons);
        primitivisedObjects->polylines.append(group->polylines);
        primitivisedObjects->points.append(group->points);

        // Empty groups are also inserted, to indicate that they are empty
        primitivisedObjects->primitivesGroups.push_back(qMove(group));
    }

    int failCounter = 0;
    // Wait for future primitives groups
    Stopwatch futureSharedPrimitivesGroupsStopwatch(metric != nullptr);
    for (auto& futureSharedGroup : futureSharedPrimitivesGroups)
    {
        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureSharedGroup.wait_until(timeout))
        {
            auto group = futureSharedGroup.get();

            // Add polygons, polylines and points from group to current context
            primitivisedObjects->polygons.append(group->polygons);
            primitivisedObjects->polylines.append(group->polylines);
            primitivisedObjects->points.append(group->points);

            // Add shared group to current context
            primitivisedObjects->primitivesGroups.push_back(qMove(group));
        }
        else
        {
            failCounter++;
            LogPrintf(LogSeverityLevel::Error, "Get futureSharedGroup timeout exceed = %d", failCounter);
        }
        if (failCounter >= 3)
        {
            LogPrintf(LogSeverityLevel::Error, "Get futureSharedGroup timeout exceed");
            break;
        }
    }
    if (metric)
        metric->elapsedTimeForFutureSharedPrimitivesGroups += futureSharedPrimitivesGroupsStopwatch.elapsed();

    if (metric)
        metric->elapsedTimeForPrimitives += obtainPrimitivesStopwatch.elapsed();
}

std::shared_ptr<const OsmAnd::MapPrimitiviser_P::PrimitivesGroup> OsmAnd::MapPrimitiviser_P::obtainPrimitivesGroup(
    const Context& context,
    const float detailScaleFactor,    
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const MapObject>& mapObject,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& orderEvaluator,
    MapStyleEvaluator& polygonEvaluator,
    MapStyleEvaluator& polylineEvaluator,
    MapStyleEvaluator& pointEvaluator,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;

    bool ok;

    const auto constructedGroup = new PrimitivesGroup(mapObject);
    std::shared_ptr<const PrimitivesGroup> group(constructedGroup);

    //////////////////////////////////////////////////////////////////////////
    //if (mapObject->toString().contains("47894962"))
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    bool skipUnclosedPolygon = !mapObject->containsTag("boundary");
    double doubledPolygonArea31 = -1.0;
    Nullable<bool> rejectByArea;

    // Setup mapObject-specific input data
    const auto layerType = mapObject->getLayerType();
    const quint64 zbSignature = zbFlagB() ? zbObjectSignature(mapObject) : 0;
    orderEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, static_cast<int>(layerType));
    orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_AREA, mapObject->isArea);
    orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_POINT, mapObject->points31.size() == 1);
    orderEvaluator.setBooleanValue(env->styleBuiltinValueDefs->id_INPUT_CYCLE, mapObject->isClosedFigure());
    polylineEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_LAYER, static_cast<int>(layerType));

    const auto& decRules = mapObject->attributeMapping->decodeMap;
    auto pAttributeId = mapObject->attributeIds.constData();
    const auto attributeIdsCount = mapObject->attributeIds.size();
    for (auto attributeIdIndex = 0; attributeIdIndex < attributeIdsCount; attributeIdIndex++, pAttributeId++)
    {
        const auto& decodedAttribute = decRules[*pAttributeId];

        //////////////////////////////////////////////////////////////////////////
        //if (mapObject->toString().contains("49048972"))
        //{
        //    const auto t = mapObject->toString();
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        const Stopwatch orderEvaluationStopwatch(metric != nullptr);

        // Setup tag+value-specific input data
        IMapStyle::StringId tagStringId;
        IMapStyle::StringId valueStringId;
        const QPair<const void*, const void*> zbAttrKey(&decodedAttribute, env.get());
        const auto zbParsed = zbFlagB() ? zbParsedTagValues().constFind(zbAttrKey) : zbParsedTagValues().cend();
        if (zbFlagB() && zbParsed != zbParsedTagValues().cend())
        {
            tagStringId = zbParsed->first;
            valueStringId = zbParsed->second;
        }
        else
        {
        MapStyleConstantValue parsedTag;
        tagStringId = env->mapStyle->parseValue(
            decodedAttribute.tag,
            env->styleBuiltinValueDefs->id_INPUT_TAG,
            parsedTag)
            ? parsedTag.asSimple.asUInt
            : std::numeric_limits<IMapStyle::StringId>::max();
        MapStyleConstantValue parsedValue;
        valueStringId = env->mapStyle->parseValue(
            decodedAttribute.value,
            env->styleBuiltinValueDefs->id_INPUT_VALUE,
            parsedValue)
            ? parsedValue.asSimple.asUInt
            : std::numeric_limits<IMapStyle::StringId>::max();
            if (zbFlagB())
                zbParsedTagValues().insert(zbAttrKey, qMakePair<quint32, quint32>(tagStringId, valueStringId));
        }
        orderEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_TAG, tagStringId);
        orderEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, valueStringId);

        evaluationResult.clear();
        ok = zbEvaluate(orderEvaluator, mapObject, MapStyleRulesetType::Order, evaluationResult, zbSignature, tagStringId, valueStringId, env.get(), context.zoom);

        if (metric)
        {
            metric->elapsedTimeForOrderEvaluation += orderEvaluationStopwatch.elapsed();
            metric->orderEvaluations++;
        }

        const Stopwatch orderProcessingStopwatch(metric != nullptr);

        // If evaluation failed, skip
        if (!ok)
        {
            if (metric)
            {
                metric->elapsedTimeForOrderProcessing += orderProcessingStopwatch.elapsed();
                metric->orderRejects++;
            }

            continue;
        }

        int objectType_;
        if (!evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_OBJECT_TYPE, objectType_))
        {
            if (metric)
            {
                metric->elapsedTimeForOrderProcessing += orderProcessingStopwatch.elapsed();
                metric->orderRejects++;
            }

            continue;
        }
        const auto objectType = static_cast<PrimitiveType>(objectType_);

        int zOrder = -1;
        if (!evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_ORDER, zOrder) || zOrder < 0)
        {
            if (metric)
            {
                metric->elapsedTimeForOrderProcessing += orderProcessingStopwatch.elapsed();
                metric->orderRejects++;
            }

            continue;
        }

        int shadowLevel = 0;
        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_SHADOW_LEVEL, shadowLevel);

        if (objectType == PrimitiveType::Polygon)
        {
            const Stopwatch polygonProcessingStopwatch(metric != nullptr);

            // Perform checks on data
            if (mapObject->points31.size() <= 2)
            {
                if (metric)
                    metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but has only %d point(s)",
                    qPrintable(mapObject->toString()),
                    mapObject->points31.size());
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }
            if (skipUnclosedPolygon && !mapObject->isClosedFigure())
            {
                if (metric)
                    metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but isn't closed",
                    qPrintable(mapObject->toString()));
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }
            if (skipUnclosedPolygon && !mapObject->isClosedFigure(true))
            {
                if (metric)
                    metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s primitive is processed as polygon, but isn't closed (inner)",
                    qPrintable(mapObject->toString()));
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            //if ((mapObject->id >> 1) == 266877135u)
            //{
            //    int i = 5;
            //}
            //////////////////////////////////////////////////////////////////////////

            // Check size of polygon
            auto ignorePolygonArea = false;
            evaluationResult.getBooleanValue(
                env->styleBuiltinValueDefs->id_OUTPUT_IGNORE_POLYGON_AREA,
                ignorePolygonArea);

            auto ignorePolygonAsPointArea = false;
            evaluationResult.getBooleanValue(
                env->styleBuiltinValueDefs->id_OUTPUT_IGNORE_POLYGON_AS_POINT_AREA,
                ignorePolygonAsPointArea);

            if (doubledPolygonArea31 < 0.0)
                doubledPolygonArea31 = Utilities::doubledPolygonArea(mapObject->points31);

            if ((!ignorePolygonArea || !ignorePolygonAsPointArea) && !rejectByArea.isSet())
            {
                const auto polygonArea31 = static_cast<double>(doubledPolygonArea31)* 0.5;
                const auto areaScaleDivisor31ToPixel =
                    primitivisedObjects->scaleDivisor31ToPixel.x * primitivisedObjects->scaleDivisor31ToPixel.y;
                const auto polygonAreaInPixels = polygonArea31 / areaScaleDivisor31ToPixel;
                const auto polygonAreaInAbstractPixels =
                    polygonAreaInPixels / (env->displayDensityFactor * env->displayDensityFactor);
                rejectByArea =
                    primitivisedObjects->scaleDivisor31ToPixel.x >= 0.0 &&
                    primitivisedObjects->scaleDivisor31ToPixel.y >= 0.0 &&
                    polygonAreaInAbstractPixels <= context.polygonAreaMinimalThreshold;
            }

            if (!rejectByArea.isSet() || *rejectByArea.getValuePtrOrNullptr() == false || ignorePolygonArea)
            {
                const Stopwatch polygonEvaluationStopwatch(metric != nullptr);

                // Setup tag+value-specific input data (for Polygon)
                polygonEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_TAG, tagStringId);
                polygonEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, valueStringId);

                // Evaluate style for this primitive to check if it passes (for Polygon)
                evaluationResult.clear();
                ok = zbEvaluate(polygonEvaluator, mapObject, MapStyleRulesetType::Polygon, evaluationResult, zbSignature, tagStringId, valueStringId, env.get(), context.zoom);

                if (metric)
                {
                    metric->elapsedTimeForPolygonEvaluation += polygonEvaluationStopwatch.elapsed();
                    metric->polygonEvaluations++;
                }

                // Add as polygon if accepted as polygon
                if (ok)
                {
                    // Create new primitive
                    const std::shared_ptr<Primitive> primitive(new Primitive(
                        group,
                        objectType,
                        attributeIdIndex,
                        evaluationResult));
                    primitive->zOrder = (std::dynamic_pointer_cast<const SurfaceMapObject>(mapObject) || std::dynamic_pointer_cast<const CoastlineMapObject>(mapObject))
                        ? std::numeric_limits<int>::min()
                        : zOrder;
                    primitive->doubledArea = doubledPolygonArea31;

                    // Accept this primitive
                    if (zOrder <= MAX_POLYGON_ORDER_IN_POLYGONS_LIST)
                        constructedGroup->polygons.push_back(qMove(primitive));
                    else
                        // Add to polylines to respect order while rastering                
                        constructedGroup->polylines.push_back(qMove(primitive));

                    // Update metric
                    if (metric)
                        metric->polygonPrimitives++;
                }
                else
                {
                    if (metric)
                        metric->polygonRejects++;
                }
            }
            else
            {
                if (metric)
                    metric->polygonsRejectedByArea++;
            }

            if (metric)
                metric->elapsedTimeForPolygonProcessing += polygonProcessingStopwatch.elapsed();

            if (!rejectByArea.isSet() || *rejectByArea.getValuePtrOrNullptr() == false || ignorePolygonAsPointArea)
            {
                const Stopwatch pointEvaluationStopwatch(metric != nullptr);

                // Setup tag+value-specific input data (for Point)
                pointEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_TAG, tagStringId);
                pointEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, valueStringId);

                // Icon also depends on native text length
                const auto& nativeCaption = mapObject->getCaptionInNativeLanguage();
                int nativeCaptionLength = nativeCaption.isNull() ? 0 : nativeCaption.length();
                pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, nativeCaptionLength);

                // Evaluate Point rules
                evaluationResult.clear();
                const auto hasIcon = zbEvaluate(pointEvaluator, mapObject, MapStyleRulesetType::Point, evaluationResult, zbSignature, tagStringId, valueStringId, env.get(), context.zoom);

                // Update metric
                if (metric)
                {
                    metric->elapsedTimeForPointEvaluation += pointEvaluationStopwatch.elapsed();
                    metric->pointEvaluations++;
                }

                const Stopwatch pointProcessingStopwatch(metric != nullptr);

                // Create point primitive only in case polygon has any content
                if (!mapObject->captions.isEmpty() || hasIcon)
                {
                    // Duplicate primitive as point
                    std::shared_ptr<Primitive> pointPrimitive;
                    if (hasIcon)
                    {
                        // Point evaluation is a bit special, it's success only indicates that point has an icon
                        pointPrimitive.reset(new Primitive(
                            group,
                            PrimitiveType::Point,
                            attributeIdIndex,
                            evaluationResult));
                    }
                    else
                    {
                        pointPrimitive.reset(new Primitive(
                            group,
                            PrimitiveType::Point,
                            attributeIdIndex));
                    }
                    pointPrimitive->zOrder = (std::dynamic_pointer_cast<const SurfaceMapObject>(mapObject) || std::dynamic_pointer_cast<const CoastlineMapObject>(mapObject))
                        ? std::numeric_limits<int>::min()
                        : zOrder;
                    pointPrimitive->doubledArea = doubledPolygonArea31;

                    constructedGroup->points.push_back(qMove(pointPrimitive));

                    // Update metric
                    if (metric)
                        metric->pointPrimitives++;
                }

                if (metric)
                    metric->elapsedTimeForPointProcessing += pointProcessingStopwatch.elapsed();
            }
        }
        else if (objectType == PrimitiveType::Polyline)
        {
            // Perform checks on data
            if (mapObject->points31.size() < 2)
            {
#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s is processed as polyline, but has %d point(s)",
                    qPrintable(mapObject->toString()),
                    mapObject->points31.size());
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }

            const Stopwatch polylineEvaluationStopwatch(metric != nullptr);

            // Setup tag+value-specific input data
            polylineEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_TAG, tagStringId);
            polylineEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, valueStringId);

            // Evaluate style for this primitive to check if it passes
            evaluationResult.clear();
            ok = zbEvaluate(polylineEvaluator, mapObject, MapStyleRulesetType::Polyline, evaluationResult, zbSignature, tagStringId, valueStringId, env.get(), context.zoom);

            if (metric)
            {
                metric->elapsedTimeForPolylineEvaluation += polylineEvaluationStopwatch.elapsed();
                metric->polylineEvaluations++;
            }

            const Stopwatch polylineProcessingStopwatch(metric != nullptr);

            // If evaluation failed, skip
            if (!ok)
            {
                if (metric)
                {
                    metric->elapsedTimeForPolylineProcessing += polylineProcessingStopwatch.elapsed();
                    metric->polylineRejects++;
                }

                continue;
            }

            // Create new primitive
            const std::shared_ptr<Primitive> primitive(new Primitive(
                group,
                objectType,
                attributeIdIndex,
                evaluationResult,
                detailScaleFactor));
            primitive->zOrder = zOrder;

            // Accept this primitive
            if (zOrder >= MIN_POLYLINE_ORDER_IN_POLYLINES_LIST)
                constructedGroup->polylines.push_back(qMove(primitive));
            else
                // Add to polygons to respect order while rasterizing
                constructedGroup->polygons.push_back(qMove(primitive));

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPolylineProcessing += polylineProcessingStopwatch.elapsed();
                metric->polylinePrimitives++;
            }
        }
        else if (objectType == PrimitiveType::Point)
        {
            // Perform checks on data
            if (mapObject->points31.size() < 1)
            {
#if OSMAND_VERBOSE_MAP_PRIMITIVISER
                LogPrintf(LogSeverityLevel::Warning,
                    "MapObject %s is processed as point, but has no point",
                    qPrintable(mapObject->toString()));
#endif // OSMAND_VERBOSE_MAP_PRIMITIVISER
                continue;
            }

            const Stopwatch pointEvaluationStopwatch(metric != nullptr);

            // Setup tag+value-specific input data
            pointEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_TAG, tagStringId);
            pointEvaluator.setStringIdValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, valueStringId);

            // Icon also depends on native text length
            const auto& nativeCaption = mapObject->getCaptionInNativeLanguage();
            int nativeCaptionLength = nativeCaption.isNull() ? 0 : nativeCaption.length();
            pointEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, nativeCaptionLength);

            // Evaluate Point rules
            evaluationResult.clear();
            const bool hasIcon = zbEvaluate(pointEvaluator, mapObject, MapStyleRulesetType::Point, evaluationResult, zbSignature, tagStringId, valueStringId, env.get(), context.zoom);

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPointEvaluation += pointEvaluationStopwatch.elapsed();
                metric->pointEvaluations++;
            }

            const Stopwatch pointProcessingStopwatch(metric != nullptr);

            // Create point primitive only in case polygon has any content
            if (mapObject->captions.isEmpty() && !hasIcon)
            {
                if (metric)
                {
                    metric->elapsedTimeForPointProcessing += pointProcessingStopwatch.elapsed();
                    metric->pointRejects++;
                }

                continue;
            }

            // Create new primitive
            std::shared_ptr<Primitive> primitive;
            if (hasIcon)
            {
                // Point evaluation is a bit special, it's success only indicates that point has an icon
                primitive.reset(new Primitive(
                    group,
                    PrimitiveType::Point,
                    attributeIdIndex,
                    evaluationResult));
            }
            else
            {
                primitive.reset(new Primitive(
                    group,
                    PrimitiveType::Point,
                    attributeIdIndex));
            }
            primitive->zOrder = zOrder;

            // Accept this primitive
            constructedGroup->points.push_back(qMove(primitive));

            // Update metric
            if (metric)
            {
                metric->elapsedTimeForPointProcessing += pointProcessingStopwatch.elapsed();
                metric->pointPrimitives++;
            }
        }
        else
        {
            assert(false);
            continue;
        }

        //if (shadowLevel > 0)
        //{
        //    primitivisedObjects->shadowLevelMin = qMin(primitivisedObjects->shadowLevelMin, static_cast<int>(zOrder));
        //    primitivisedObjects->shadowLevelMax = qMax(primitivisedObjects->shadowLevelMax, static_cast<int>(zOrder));
        //}
    }

    return group;
}

void OsmAnd::MapPrimitiviser_P::sortAndFilterPrimitives(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const MapObject::Comparator mapObjectsComparator;
    const auto privitivesSort =
        [mapObjectsComparator]
        (const std::shared_ptr<const Primitive>& l, const std::shared_ptr<const Primitive>& r) -> bool
        {
            // Sort by zOrder first
            if (l->zOrder != r->zOrder)
                return l->zOrder < r->zOrder;

            // Then sort by area
            if (l->doubledArea != r->doubledArea)
                return l->doubledArea > r->doubledArea;

            // Then sort by tag=value ordering (this is possible only inside same object)
            if (l->attributeIdIndex != r->attributeIdIndex && l->sourceObject == r->sourceObject)
            {
                if (l->type == PrimitiveType::Polygon)
                    return l->attributeIdIndex > r->attributeIdIndex;
                return l->attributeIdIndex < r->attributeIdIndex;
            }

            // Then sort by number of points
            const auto lPointsCount = l->sourceObject->points31.size();
            const auto rPointsCount = r->sourceObject->points31.size();
            if (lPointsCount != rPointsCount)
                return lPointsCount < rPointsCount;

            // Sort by map object sorting key, if map objects are equal
            return mapObjectsComparator(l->sourceObject, r->sourceObject);
        };

    std::sort(primitivisedObjects->polygons, privitivesSort);
    std::sort(primitivisedObjects->polylines, privitivesSort);
    filterOutHighwaysByDensity(context, primitivisedObjects, metric);
    std::sort(primitivisedObjects->points, privitivesSort);
}

void OsmAnd::MapPrimitiviser_P::filterOutHighwaysByDensity(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    // Check if any filtering needed
    if (context.roadDensityZoomTile == 0 || context.roadsDensityLimitPerTile == 0)
        return;

    const auto dZ = primitivisedObjects->zoom + context.roadDensityZoomTile;
    QHash< uint64_t, std::pair<uint32_t, double> > densityMap;

    auto itPolyline = mutableIteratorOf(primitivisedObjects->polylines);
    itPolyline.toBack();
    while (itPolyline.hasPrevious())
    {
        const auto& polyline = itPolyline.previous();
        const auto& sourceObject = polyline->sourceObject;

        // If polyline is not road, it should be accepted
        const auto pPrimitiveAttribute = sourceObject->resolveAttributeByIndex(polyline->attributeIdIndex);
        if (!pPrimitiveAttribute)
            continue;
        if (pPrimitiveAttribute->tag != QLatin1String("highway"))
            continue;

        auto accept = false;
        uint64_t prevId = 0;
        const auto pointsCount = sourceObject->points31.size();
        auto pPoint = sourceObject->points31.constData();
        for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
        {
            auto x = pPoint->x >> (MaxZoomLevel - dZ);
            auto y = pPoint->y >> (MaxZoomLevel - dZ);
            uint64_t id = (static_cast<uint64_t>(x) << dZ) | y;
            if (prevId != id)
            {
                prevId = id;

                auto& mapEntry = densityMap[id];
                if (mapEntry.first < context.roadsDensityLimitPerTile /*&& p.second > o */)
                {
                    accept = true;
                    mapEntry.first += 1;
                    mapEntry.second = polyline->zOrder;
                }
            }
        }
        if (!accept)
        {
            if (metric)
                metric->polylineRejectedByDensity++;

            itPolyline.remove();
        }
    }
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitivesSymbols(
    const Context& context,
    const std::shared_ptr<PrimitivisedObjects>& primitivisedObjects,
    const TileId tileId,
    MapStyleEvaluationResult& evaluationResult,
    const std::shared_ptr<Cache>& cache,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;
    const auto zoom = primitivisedObjects->zoom;

    // Initialize shared settings for text evaluation
    MapStyleEvaluator textEvaluator(env->mapStyle, env->displayDensityFactor * env->symbolsScaleFactor);
    env->applyTo(textEvaluator);
    textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MINZOOM, zoom);
    textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_MAXZOOM, zoom);

    // Local cache to speed up textOrder evaluation (based on name tag only!)
    QHash<QString, int> textOrderCache;

    //NOTE: Em, I'm not sure this is still true
    //NOTE: Since 2 tiles with same MapObject may have different set of polylines, generated from it,
    //NOTE: then set of symbols also should differ, but it won't.
    const auto pSharedSymbolGroups = cache ? cache->getSymbolsGroupsPtr(primitivisedObjects->zoom) : nullptr;
    QList< proper::shared_future< std::shared_ptr<const SymbolsGroup> > > futureSharedSymbolGroups;
    for (const auto& primitivesGroup : constOf(primitivisedObjects->primitivesGroups))
    {
        if (queryController && queryController->isAborted())
            return;

        // If using shared context is allowed, check if this group was already processed
        // (using shared cache is only allowed for non-generated MapObjects),
        // then symbols group can be shared (excluding labeled polygons)
        MapObject::SharingKey sharingKey;
        const bool isLabeledPolygon = !primitivesGroup->polygons.isEmpty() && !primitivesGroup->points.isEmpty();
        const auto canBeShared = primitivesGroup->sourceObject->obtainSharingKey(sharingKey) && !isLabeledPolygon;

        //////////////////////////////////////////////////////////////////////////
        //if ((primitivesGroup->sourceObject->id >> 1) == 1937897178u)
        //{
        //    int i = 5;
        //}
        //////////////////////////////////////////////////////////////////////////

        if (pSharedSymbolGroups && canBeShared)
        {
            // If this group was already processed, use that
            std::shared_ptr<const SymbolsGroup> group;
            proper::shared_future< std::shared_ptr<const SymbolsGroup> > futureGroup;
            if (pSharedSymbolGroups->obtainReferenceOrFutureReferenceOrMakePromise(sharingKey, group, futureGroup))
            {
                if (group)
                {
                    // Add shared group to current context
                    assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
                    primitivisedObjects->symbolsGroups.insert(group->sourceObject, qMove(group));
                }
                else
                {
                    futureSharedSymbolGroups.push_back(qMove(futureGroup));
                }

                continue;
            }
        }

        const Stopwatch symbolsGroupsProcessingStopwatch(metric != nullptr);

        // Allow displaying only captions for multi-labeled polygons
        const bool canBeShownWithoutIcon = isLabeledPolygon && primitivesGroup->points.size() > 1;

        // Create a symbols group
        const std::shared_ptr<SymbolsGroup> group(new SymbolsGroup(
            primitivesGroup->sourceObject, canBeShownWithoutIcon));

        // For each primitive if primitive group, collect symbols from it
        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            tileId,
            primitivesGroup->polygons,
            evaluationResult,
            textEvaluator,
            textOrderCache,
            group->symbols,
            queryController,
            metric);

        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            tileId,
            primitivesGroup->polylines,
            evaluationResult,
            textEvaluator,
            textOrderCache,
            group->symbols,
            queryController,
            metric);

        collectSymbolsFromPrimitives(
            context,
            primitivisedObjects,
            tileId,
            primitivesGroup->points,
            evaluationResult,
            textEvaluator,
            textOrderCache,
            group->symbols,
            queryController,
            metric);

        // Update metric
        if (metric)
        {
            metric->elapsedTimeForSymbolsGroupsProcessing += symbolsGroupsProcessingStopwatch.elapsed();
            metric->symbolsGroupsProcessed++;
        }
        
        // Add this group to shared cache
        if (pSharedSymbolGroups && canBeShared)
            pSharedSymbolGroups->fulfilPromiseAndReference(sharingKey, group);
        
        // Empty groups are also inserted, to indicate that they are empty
        assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
        primitivisedObjects->symbolsGroups.insert(group->sourceObject, group);
    }

    int failCounter = 0;
    for (auto& futureGroup : futureSharedSymbolGroups)
    {
        auto timeout = proper::chrono::system_clock::now() + proper::chrono::seconds(3);
        if (proper::future_status::ready == futureGroup.wait_until(timeout))
        {
            const auto group = futureGroup.get();

            // Add shared group to current context
            assert(!primitivisedObjects->symbolsGroups.contains(group->sourceObject));
            primitivisedObjects->symbolsGroups.insert(group->sourceObject, group);
        }
        else
        {
            failCounter++;
            LogPrintf(LogSeverityLevel::Error, "Get futureGroup timeout exceed = %d", failCounter);
        }
        if (failCounter >= 3)
        {
            LogPrintf(LogSeverityLevel::Error, "Get futureGroup timeout exceed");
            break;
        }
    }
}

void OsmAnd::MapPrimitiviser_P::collectSymbolsFromPrimitives(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const TileId tileId,
    const PrimitivesCollection& primitives,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    QHash<QString, int>& textOrderCache,
    SymbolsCollection& outSymbols,
    const std::shared_ptr<const IQueryController>& queryController,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    for (const auto& primitive : constOf(primitives))
    {
        //////////////////////////////////////////////////////////////////////////
        //if (primitive->sourceObject->toString().contains("49048972"))
        //{
        //    const auto t = primitive->sourceObject->toString();
        //    int i = 5;
        //}
        //else
        //    return;
        //////////////////////////////////////////////////////////////////////////

        if (queryController && queryController->isAborted())
            return;

        //NOTE: Each polygon that has icon or text is also added as point. So there's no need to process polygons
        if (primitive->type == PrimitiveType::Polyline)
        {
            obtainSymbolsFromPolyline(
                context,
                primitivisedObjects,
                primitive,
                evaluationResult,
                textEvaluator,
                textOrderCache,
                outSymbols,
                metric);
        }
        else if (primitive->type == PrimitiveType::Point)
        {
            const auto& attributeMapping = primitive->sourceObject->attributeMapping;
            const auto attributeId = primitive->sourceObject->attributeIds[primitive->attributeIdIndex];
            const auto& decodedAttribute = attributeMapping->decodeMap[attributeId];
            if (!context.env->disabledAttributes.contains(decodedAttribute.value))
            {
                obtainSymbolsFromPoint(
                    context,
                    primitivisedObjects,
                    tileId,
                    primitive,
                    evaluationResult,
                    textEvaluator,
                    textOrderCache,
                    outSymbols,
                    metric);
            }
        }
    }
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPolygon(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    QHash<QString, int>& textOrderCache,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() > 2);
    // assert(primitive->sourceObject->isClosedFigure());
    // assert(primitive->sourceObject->isClosedFigure(true));

    // Get center of polygon, since all symbols of polygon are related to it's center
    PointI64 center;
    const auto pointsCount = points31.size();
    auto pPoint = points31.constData();
    for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
    {
        center.x += pPoint->x;
        center.y += pPoint->y;
    }
    center.x /= pointsCount;
    center.y /= pointsCount;

    // Obtain texts for this symbol
    obtainPrimitiveTexts(
        context,
        primitivisedObjects,
        primitive,
        Utilities::normalizeCoordinates(center, ZoomLevel31),
        evaluationResult,
        textEvaluator,
        textOrderCache,
        outSymbols,
        metric);
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPolyline(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    QHash<QString, int>& textOrderCache,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() >= 2);

    // Symbols for polyline are always related to it's "middle" point
    const auto center = points31[points31.size() >> 1];

    // Obtain texts for this symbol
    obtainPrimitiveTexts(
        context,
        primitivisedObjects,
        primitive,
        center,
        evaluationResult,
        textEvaluator,
        textOrderCache,
        outSymbols,
        metric);
}

void OsmAnd::MapPrimitiviser_P::obtainSymbolsFromPoint(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const TileId tileId,
    const std::shared_ptr<const Primitive>& primitive,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    QHash<QString, int>& textOrderCache,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString().contains("49048972"))
    //{
    //    const auto t = primitive->sourceObject->toString();
    //    int i = 5;
    //}
    //else
    //    return;
    //////////////////////////////////////////////////////////////////////////

    const auto& points31 = primitive->sourceObject->points31;

    assert(points31.size() > 0);

    // Depending on type of point, center is determined differently
    PointI center;
    if (primitive->sourceObject->isLabelCoordinatesSpecified())
    {
        center.x = primitive->sourceObject->getLabelCoordinateX();
        center.y = primitive->sourceObject->getLabelCoordinateY();
    }
    else if (points31.size() == 1)
    {
        // Regular point
        center = points31.first();
    }
    else
    {
        // Point represents center of polygon
        PointI64 center_;
        const auto pointsCount = points31.size();
        auto pPoint = points31.constData();
        for (auto pointIdx = 0; pointIdx < pointsCount; pointIdx++, pPoint++)
        {
            center_.x += pPoint->x;
            center_.y += pPoint->y;
        }
        center_.x /= pointsCount;
        center_.y /= pointsCount;

        center = Utilities::normalizeCoordinates(center_, ZoomLevel31);
    }

    // Don't process symbols from labeled polygons if label positions are out of this tile
    if (points31.size() != 1)
    {
        const auto zoomShift = MaxZoomLevel - primitivisedObjects->zoom;
        const auto tileSize = static_cast<int>((1u << zoomShift) - 1u);
        const PointI topLeft(tileId.x << zoomShift, tileId.y << zoomShift);
        const PointI botRight(topLeft.x + tileSize, topLeft.y + tileSize);
        if (center.x < topLeft.x || center.y < topLeft.y || center.x > botRight.x || center.y > botRight.y)
            return;
    }

    obtainPrimitiveIcon(
        context,
        primitive,
        center,
        evaluationResult,
        outSymbols,
        metric);

    obtainPrimitiveTexts(
        context,
        primitivisedObjects,
        primitive,
        center,
        evaluationResult,
        textEvaluator,
        textOrderCache,
        outSymbols,
        metric);
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitiveTexts(
    const Context& context,
    const std::shared_ptr<const PrimitivisedObjects>& primitivisedObjects,
    const std::shared_ptr<const Primitive>& primitive,
    const PointI& location,
    MapStyleEvaluationResult& evaluationResult,
    MapStyleEvaluator& textEvaluator,
    QHash<QString, int>& textOrderCache,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& mapObject = primitive->sourceObject;
    const auto& env = context.env;

    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString().contains("47894962"))
    //{
    //    int i = 5;
    //}
    //////////////////////////////////////////////////////////////////////////

    // Text symbols can only be obtained from captions
    if (mapObject->captions.isEmpty())
        return;

    const auto& attributeMapping = mapObject->attributeMapping;
    const auto attributeId = mapObject->attributeIds[primitive->attributeIdIndex];
    const auto& decodedAttribute = attributeMapping->decodeMap[attributeId];

    // Set common evaluator settings
    textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_TAG, decodedAttribute.tag);
    textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_VALUE, decodedAttribute.value);
    QString tagWithValue = decodedAttribute.tag;
    tagWithValue.append(QStringLiteral(" = "));
    tagWithValue.append(decodedAttribute.value);
    tagWithValue.append(QStringLiteral(", "));

    // Get captions and their order
    auto captions = mapObject->captions;
    auto captionsOrder = mapObject->captionsOrder;

    // Process captions to find out what names are present and modify that if needed
    bool hasNativeName = false;
    bool hasLocalizedName = false;
    bool hasEnglishName = false;
    uint32_t localizedNameRuleId = std::numeric_limits<uint32_t>::max();
    const auto citCaptionsEnd = captions.cend();

    // Look for localized name
    const auto& localeLanguageId = env->getLocaleLanguageId().toLower();
    const auto citLocalizedNameRuleId = attributeMapping->localizedNameAttributes.constFind(&localeLanguageId);
    if (citLocalizedNameRuleId != attributeMapping->localizedNameAttributes.cend())
        localizedNameRuleId = *citLocalizedNameRuleId;

    // In transliterating modes, use English name as localized if both native and localized names are missing
    const auto languagePreference = env->getLanguagePreference();
    const bool transliterate =
        languagePreference == MapPresentationEnvironment::LanguagePreference::NativeAndLocalizedOrTransliterated ||
        languagePreference == MapPresentationEnvironment::LanguagePreference::LocalizedOrTransliteratedAndNative ||
        languagePreference == MapPresentationEnvironment::LanguagePreference::LocalizedOrTransliterated;
    if (transliterate &&
        !captions.contains(localizedNameRuleId) &&
        !captions.contains(attributeMapping->nativeNameAttributeId) &&
        captions.contains(attributeMapping->enNameAttributeId))
    {
        localizedNameRuleId = attributeMapping->enNameAttributeId;
    }
    
    // sort captionsOrder by textOrder property
    QHash<uint32_t, int> textOrderMap;

    int textOrder;

    for (const auto& captionAttributeId : constOf(captionsOrder))
    {
        QString captionAttributeTag;
        if (captionAttributeId != attributeMapping->nativeNameAttributeId && captionAttributeId != localizedNameRuleId)
            captionAttributeTag = attributeMapping->decodeMap[captionAttributeId].tag;
        QString cacheKey = tagWithValue;
        cacheKey.append(captionAttributeTag);
        if (!textOrderCache.contains(cacheKey))
        {
            textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_NAME_TAG, captionAttributeTag);
            evaluationResult.clear();
            textEvaluator.evaluate(mapObject, MapStyleRulesetType::Text, &evaluationResult);
            textOrder = 100;
            evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ORDER, textOrder);
            textOrderCache.insert(cacheKey, textOrder);
        }
        else
            textOrder = textOrderCache.value(cacheKey);
        textOrderMap.insert(captionAttributeId, textOrder);
    }
    std::sort(captionsOrder.begin(), captionsOrder.end(), [&textOrderMap] (uint32_t c1, uint32_t c2) {
        return textOrderMap.value(c1) < textOrderMap.value(c2);
    });

    // Look for native name
    auto citNativeName =
        (attributeMapping->nativeNameAttributeId == std::numeric_limits<uint32_t>::max())
        ? citCaptionsEnd
        : captions.constFind(attributeMapping->nativeNameAttributeId);
    hasNativeName = (citNativeName != citCaptionsEnd);
    auto nativeNameOrder = hasNativeName
        ? captionsOrder.indexOf(citNativeName.key())
        : -1;
    
    auto citLocalizedName =
        (localizedNameRuleId == std::numeric_limits<uint32_t>::max())
        ? citCaptionsEnd
        : captions.constFind(localizedNameRuleId);
    hasLocalizedName = (citLocalizedName != citCaptionsEnd);
    auto localizedNameOrder = hasLocalizedName
        ? captionsOrder.indexOf(citLocalizedName.key())
        : -1;
    
    auto citEnglishName =
        (attributeMapping->enNameAttributeId == std::numeric_limits<uint32_t>::max())
        ? citCaptionsEnd
        : captions.constFind(attributeMapping->enNameAttributeId);
    hasEnglishName = (citEnglishName != citCaptionsEnd);
    

    // According to presentation settings, adjust set of captions
    switch (languagePreference)
    {
        case MapPresentationEnvironment::LanguagePreference::NativeOnly:
        {
            // Remove localized name if such exists
            if (hasLocalizedName)
            {
                captions.remove(localizedNameRuleId);
                captionsOrder.removeOne(localizedNameRuleId);
                hasLocalizedName = false;
                citLocalizedName = citCaptionsEnd;
                localizedNameOrder = -1;
            }
            break;
        }

        case MapPresentationEnvironment::LanguagePreference::LocalizedOrNative:
        {
            // Only one should be shown, thus remove native if localized exist
            if (hasLocalizedName && hasNativeName)
            {
                captions.remove(attributeMapping->nativeNameAttributeId);
                captionsOrder.removeOne(attributeMapping->nativeNameAttributeId);
                hasNativeName = false;
                citNativeName = citCaptionsEnd;
                nativeNameOrder = -1;
            }
            break;
        }

        case MapPresentationEnvironment::LanguagePreference::NativeAndLocalized:
        {
            // Both should be shown if available, and in correct order
            if (hasNativeName && hasLocalizedName && localizedNameOrder < nativeNameOrder)
            {
                captionsOrder.swap(localizedNameOrder, nativeNameOrder);
                qSwap(localizedNameOrder, nativeNameOrder);
            }
            break;
        }

        case MapPresentationEnvironment::LanguagePreference::NativeAndLocalizedOrTransliterated:
        {
            // Both should be present, so add transliterated in case localized is missing, and in correct order
            if (hasNativeName && hasLocalizedName && localizedNameOrder < nativeNameOrder)
            {
                captionsOrder.swap(localizedNameOrder, nativeNameOrder);
                qSwap(localizedNameOrder, nativeNameOrder);
            }
            else if (hasNativeName && !hasLocalizedName)
            {
                const auto latinNameValue = (hasEnglishName) ? citEnglishName.value() : ICU::transliterateToLatin(citNativeName.value());

                citLocalizedName = captions.insert(localizedNameRuleId, latinNameValue);
                localizedNameOrder = captionsOrder.indexOf(attributeMapping->nativeNameAttributeId) + 1;
                captionsOrder.insert(localizedNameOrder, localizedNameRuleId);
                hasLocalizedName = true;
            }
            break;
        }

        case MapPresentationEnvironment::LanguagePreference::LocalizedAndNative:
        {
            // Both should be shown if available, and in correct order
            if (hasLocalizedName && hasNativeName && nativeNameOrder < localizedNameOrder)
            {
                captionsOrder.swap(nativeNameOrder, localizedNameOrder);
                qSwap(nativeNameOrder, localizedNameOrder);
            }
            break;
        }

        case MapPresentationEnvironment::LanguagePreference::LocalizedOrTransliteratedAndNative:
        {
            // Both should be present, so add transliterated in case localized is missing, and in correct order
            if (hasLocalizedName && hasNativeName && nativeNameOrder < localizedNameOrder)
            {
                captionsOrder.swap(nativeNameOrder, localizedNameOrder);
                qSwap(nativeNameOrder, localizedNameOrder);
            }
            else if (!hasLocalizedName && hasNativeName)
            {
                const auto latinNameValue = (hasEnglishName) ? citEnglishName.value() : ICU::transliterateToLatin(citNativeName.value());

                citLocalizedName = captions.insert(localizedNameRuleId, latinNameValue);
                localizedNameOrder = captionsOrder.indexOf(attributeMapping->nativeNameAttributeId);
                captionsOrder.insert(localizedNameOrder, localizedNameRuleId);
                hasLocalizedName = true;
            }
            break;
        }

        case MapPresentationEnvironment::LanguagePreference::LocalizedOrTransliterated:
        {
            // If there's no localized name, transliterate native name (if exists)
            if (!hasLocalizedName && hasNativeName)
            {
                const auto latinNameValue = (hasEnglishName) ? citEnglishName.value() : ICU::transliterateToLatin(citNativeName.value());

                citLocalizedName = captions.insert(localizedNameRuleId, latinNameValue);
                localizedNameOrder = captionsOrder.indexOf(attributeMapping->nativeNameAttributeId);
                captionsOrder.insert(localizedNameOrder, localizedNameRuleId);
                hasLocalizedName = true;
            }
            
            // Only one should be shown, thus remove native if localized exist
            if (hasLocalizedName && hasNativeName)
            {
                captions.remove(attributeMapping->nativeNameAttributeId);
                captionsOrder.removeOne(attributeMapping->nativeNameAttributeId);
                hasNativeName = false;
                citNativeName = citCaptionsEnd;
                nativeNameOrder = -1;
            }
            break;
        }

        default:
            assert(false);
            return;
    }

    // In case both languages are present, but they are equal (without accents and diacritics)
    if (hasNativeName && hasLocalizedName)
    {
        assert(citNativeName != citCaptionsEnd);
        const auto pureNativeName = ICU::stripDiacritics(citNativeName.value());

        assert(citLocalizedName != citCaptionsEnd);
        const auto pureLocalizedName = ICU::stripDiacritics(citLocalizedName.value());

        if (pureNativeName.compare(pureLocalizedName, Qt::CaseInsensitive) == 0)
        {
            // Keep first one
            if (nativeNameOrder < localizedNameOrder)
            {
                captions.remove(localizedNameRuleId);
                captionsOrder.removeOne(localizedNameRuleId);
                hasLocalizedName = false;
                citLocalizedName = citCaptionsEnd;
                localizedNameOrder = -1;
            }
            else // if (localizedNameOrder < nativeNameOrder)
            {
                captions.remove(attributeMapping->nativeNameAttributeId);
                captionsOrder.removeOne(attributeMapping->nativeNameAttributeId);
                hasNativeName = false;
                citNativeName = citCaptionsEnd;
                nativeNameOrder = -1;
            }
        }
    }

    // Process captions in order how captions where declared in OBF source (what is being controlled by 'rendering_types.xml')
    bool ok;
    bool extraCaptionTextAdded = false;
    bool isOneCaptionGrup = false;
    uint32_t extraCaptionRuleId = std::numeric_limits<uint32_t>::max();
    for (const auto& captionAttributeId : constOf(captionsOrder))
    {
        const auto& caption = constOf(captions)[captionAttributeId];

        // If it's empty, it can not be primitivised
        if (caption.isEmpty())
            continue;

        // In case this tag was already processed as extra caption, no need to duplicate it
        if (extraCaptionTextAdded && captionAttributeId == extraCaptionRuleId)
            continue;

        // Skip captions that are from 'name[:*]'-tags but not of 'native' or 'localized' language
        if (captionAttributeId != attributeMapping->nativeNameAttributeId &&
            captionAttributeId != localizedNameRuleId &&
            attributeMapping->nameAttributeIds.contains(captionAttributeId))
        {
            continue;
        }

        const Stopwatch textEvaluationStopwatch(metric != nullptr);

        // Evaluate style to obtain text parameters
        textEvaluator.setIntegerValue(env->styleBuiltinValueDefs->id_INPUT_TEXT_LENGTH, caption.length());

        // Get tag of the attribute, in case caption is not from a 'name[:*]'-attribute
        QString captionAttributeTag;
        if (captionAttributeId != attributeMapping->nativeNameAttributeId && captionAttributeId != localizedNameRuleId)
            captionAttributeTag = attributeMapping->decodeMap[captionAttributeId].tag;
        textEvaluator.setStringValue(env->styleBuiltinValueDefs->id_INPUT_NAME_TAG, captionAttributeTag);

        evaluationResult.clear();
        ok = textEvaluator.evaluate(mapObject, MapStyleRulesetType::Text, &evaluationResult);

        if (metric)
        {
            metric->elapsedTimeForTextSymbolsEvaluation += textEvaluationStopwatch.elapsed();
            metric->textSymbolsEvaluations++;
        }

        if (!ok)
            continue;

        // Skip text that doesn't have valid size
        int textSize = 0;
        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_SIZE, textSize);
        if (!ok || textSize == 0)
            continue;

        const Stopwatch textProcessingStopwatch(metric != nullptr);

        // Determine language of this text
        auto languageId = LanguageId::Invariant;
        if (hasNativeName && captionAttributeId == attributeMapping->nativeNameAttributeId)
            languageId = LanguageId::Native;
        else if (hasLocalizedName && captionAttributeId == localizedNameRuleId)
            languageId = LanguageId::Localized;

        // Create primitive
        const std::shared_ptr<TextSymbol> text(new TextSymbol(primitive));
        text->location31 = location;
        text->languageId = languageId;
        text->baseValue = caption;
        text->value = caption;
        text->scaleFactor = env->symbolsScaleFactor;

        TextSymbol::Placement placement = TextSymbol::Placement::Default;
        if (primitive->type == PrimitiveType::Point || primitive->type == PrimitiveType::Polygon)
        {
            QString placementsString;
            bool ok = evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_PLACEMENT, placementsString);
            if (ok)
            {
                const auto placementsList = placementsString.split('|', Qt::SkipEmptyParts);
                for (int i = 0; i < placementsList.size(); i++)
                {
                    const auto& placementName = placementsList[i];
                    const auto parsedPlacement = TextSymbol::placementFromString(placementName);
                    
                    if (i == 0)
                        placement = parsedPlacement;
                    else if (parsedPlacement != placement)
                        text->additionalPlacements.push_back(parsedPlacement);
                }
            }
        }
        text->placement = placement;

        bool textPositioningTaken = std::any_of(outSymbols,
            [text, isOneCaptionGrup]
            (const std::shared_ptr<const Symbol>& otherSymbol) -> bool
            {
                if (const auto otherTextSymbol = std::dynamic_pointer_cast<const TextSymbol>(otherSymbol))
                {
                    bool bothAlongPath = text->drawAlongPath && otherTextSymbol->drawAlongPath;
                    bool bothOnPath = text->drawOnPath && otherTextSymbol->drawOnPath;
                    bool bothWithoutPath = !text->drawAlongPath
                        && !text->drawOnPath 
                        && !otherTextSymbol->drawAlongPath
                        && !otherTextSymbol->drawOnPath;
                    bool equalPlacement = text->placement == otherTextSymbol->placement;

                    return bothAlongPath || bothOnPath || (bothWithoutPath && (equalPlacement && !isOneCaptionGrup));
                }
                return false;
            });
        if (textPositioningTaken)
            return;

        // Get additional text from nameTag2 if present (and not yet added)
        if (!extraCaptionTextAdded)
        {
            QString nameTag2;
            ok = evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_NAME_TAG2, nameTag2);
            if (ok && !nameTag2.isEmpty())
            {
                const auto citNameTag2AttributesGroup = attributeMapping->encodeMap.constFind(&nameTag2);
                if (citNameTag2AttributesGroup != attributeMapping->encodeMap.constEnd())
                {
                    const auto& nameTag2AttributesGroup = *citNameTag2AttributesGroup;
                    for (const auto& nameTag2AttributeEntry : rangeOf(constOf(nameTag2AttributesGroup)))
                    {
                        const auto attributeId = nameTag2AttributeEntry.value();
                        const auto citExtraCaption = mapObject->captions.constFind(attributeId);

                        if (citExtraCaption == mapObject->captions.constEnd())
                            continue;
                        const auto& extraCaption = *citExtraCaption;
                        if (extraCaption.isEmpty())
                            continue;

                        // Skip extra caption in case it's same as caption, to avoid "X (X)" cases
                        if (extraCaption == caption)
                            continue;

                        text->value += QString(QLatin1String(" (%1)")).arg(extraCaption);

                        extraCaptionTextAdded = true;
                        extraCaptionRuleId = attributeId;
                        break;
                    }
                }
            }
        }

        // In case text is from polyline primitive, it's drawn along-path if not on-path
        if (primitive->type == PrimitiveType::Polyline)
        {
            evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ON_PATH, text->drawOnPath);
            evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_TOP_SIDE, text->topSide);
            text->drawAlongPath = !text->drawOnPath;
        }

        // By default, text order is treated as 100
        text->order = 100;
        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ORDER, text->order);

        if (primitive->type == PrimitiveType::Point || primitive->type == PrimitiveType::Polygon)
            evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_DY, text->verticalOffset);

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_COLOR, text->color.argb);
        if (!ok || text->color == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            text->color = ColorARGB::fromSkColor(SK_ColorBLACK);

        text->size = textSize;

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_RADIUS, text->shadowRadius);
        if (ok && text->shadowRadius > 0)
        {
            text->shadowRadius += 2; // + 2px
            //NOTE: ^^^ This is same as specifying 'x:2' in style, but due to backward compatibility with Android, leave as-is
        }

        ok = evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_HALO_COLOR, text->shadowColor.argb);
        if (!ok || text->shadowColor == ColorARGB::fromSkColor(SK_ColorTRANSPARENT))
            text->shadowColor = ColorARGB::fromSkColor(SK_ColorWHITE);

        evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_WRAP_WIDTH, text->wrapWidth);
        if (!text->drawOnPath && text->wrapWidth == 0)
        {
            // Default wrapping width (in characters)
            text->wrapWidth = MapPrimitiviser::DefaultTextLabelWrappingLengthInCharacters;
        }

        evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_BOLD, text->isBold);
        evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_ITALIC, text->isItalic);

        ok = evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_MIN_DISTANCE, text->minDistance);
        if (!ok || qFuzzyIsNull(text->minDistance))
            text->minDistance = DEFAULT_TEXT_MIN_DISTANCE * env->displayDensityFactor * env->symbolsScaleFactor;

        evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_TEXT_SHIELD, text->shieldResourceName);
        text->shieldResourceName = prepareIconValue(primitive->sourceObject,text->shieldResourceName);

        QString iconResourceName;
        if (evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON, iconResourceName))
            text->underlayIconResourceNames.push_back(prepareIconValue(mapObject, iconResourceName));
        if (evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_2, iconResourceName))
            text->underlayIconResourceNames.push_back(prepareIconValue(mapObject, iconResourceName));

        evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_IGNORE_CLICK, text->ignoreClick);

        QString intersectsWith;
        ok = evaluationResult.getStringValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTS_WITH,
            intersectsWith);
        if (!ok)
            intersectsWith = QLatin1String("text"); // To simulate original behavior, texts should intersect only other texts
        text->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

        float intersectionSizeFactor = std::numeric_limits<float>::quiet_NaN();
        ok = evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE_FACTOR,
            intersectionSizeFactor);
        if (ok)
            text->intersectionSizeFactor = intersectionSizeFactor;

        float intersectionSize = std::numeric_limits<float>::quiet_NaN();
        ok = !ok && evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE,
            intersectionSize);
        if (ok)
            text->intersectionSize = intersectionSize;

        float intersectionMargin = std::numeric_limits<float>::quiet_NaN();
        ok = !ok && evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_MARGIN,
            intersectionMargin);
        if (ok)
            text->intersectionMargin = intersectionMargin;

        // In case new text symbol has a twin that was already added, ignore this one
        const auto hasTwin = std::any_of(outSymbols,
            [text]
            (const std::shared_ptr<const Symbol>& otherSymbol) -> bool
            {
                if (const auto otherText = std::dynamic_pointer_cast<const TextSymbol>(otherSymbol))
                {
                    return text->hasSameContentAs(*otherText);
                }

                return false;
            });

        if (hasTwin)
        {
            if (metric)
            {
                metric->elapsedTimeForTextSymbolsProcessing += textProcessingStopwatch.elapsed();
                metric->rejectedTextSymbols++;
            }

            continue;
        }

        outSymbols.push_back(qMove(text));
        isOneCaptionGrup = true;
        if (metric)
        {
            metric->elapsedTimeForTextSymbolsProcessing += textProcessingStopwatch.elapsed();
            metric->obtainedTextSymbols++;
        }
    }
}

QString OsmAnd::MapPrimitiviser_P::prepareIconValue(
    const std::shared_ptr<const MapObject>& object,
    const QString& genTagVal)
{
    return object->formatWithAdditionalAttributes(genTagVal);
}

void OsmAnd::MapPrimitiviser_P::obtainPrimitiveIcon(
    const Context& context,
    const std::shared_ptr<const Primitive>& primitive,
    const PointI& location,
    MapStyleEvaluationResult& evaluationResult,
    SymbolsCollection& outSymbols,
    MapPrimitiviser_Metrics::Metric_primitivise* const metric)
{
    const auto& env = context.env;

    //////////////////////////////////////////////////////////////////////////
    //if (primitive->sourceObject->toString().contains("1333827773"))
    //{
    //    int i = 5;
    //}
    //else
    //    return;
    //////////////////////////////////////////////////////////////////////////

    if (primitive->evaluationResult.isEmpty())
        return;

    bool ok;

    const Stopwatch iconProcessingStopwatch(metric != nullptr);

    QString iconResourceName;
    ok = primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON, iconResourceName);
    if (!ok || iconResourceName.isEmpty())
    {
        if (metric)
            metric->elapsedTimeForIconSymbolsProcessing += iconProcessingStopwatch.elapsed();

        return;
    }

    const std::shared_ptr<IconSymbol> icon(new IconSymbol(primitive));
    icon->drawAlongPath = (primitive->type == PrimitiveType::Polyline);
    icon->location31 = location;
    icon->scaleFactor = env->symbolsScaleFactor;
    
    icon->resourceName = prepareIconValue(primitive->sourceObject, iconResourceName);
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON__3, iconResourceName))
        icon->underlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON__2, iconResourceName))
        icon->underlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON__1, iconResourceName))
        icon->underlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_2, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_3, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_4, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));
    if (primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_5, iconResourceName))
        icon->overlayResourceNames.push_back(prepareIconValue(primitive->sourceObject, iconResourceName));

    //NOTE: Also divide by 2, since for some reason factor is calculated using half-size, not size
    if (primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_SHIFT_PX, icon->offsetFactor.x))
        icon->offsetFactor.x *= 0.5f;
    if (primitive->evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_SHIFT_PY, icon->offsetFactor.y))
        icon->offsetFactor.y *= 0.5f;

    icon->order = 100;
    primitive->evaluationResult.getIntegerValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_ORDER, icon->order);
    //NOTE: a magic shifting of icon order. This is needed to keep icons less important than anything else
    icon->order += 1000000;

    evaluationResult.getFloatValue(env->styleBuiltinValueDefs->id_OUTPUT_ICON_MIN_DISTANCE, icon->minDistance);

    primitive->evaluationResult.getStringValue(env->styleBuiltinValueDefs->id_OUTPUT_SHIELD, icon->shieldResourceName);

    primitive->evaluationResult.getBooleanValue(env->styleBuiltinValueDefs->id_OUTPUT_IGNORE_CLICK, icon->ignoreClick);

    QString intersectsWith;
    ok = primitive->evaluationResult.getStringValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTS_WITH,
        intersectsWith);
    if (!ok)
        intersectsWith = QLatin1String("icon"); // To simulate original behavior, icons should intersect only other icons
    icon->intersectsWith = intersectsWith.split(QLatin1Char(','), QString::SkipEmptyParts).toSet();

    float intersectionSizeFactor = std::numeric_limits<float>::quiet_NaN();
    ok = primitive->evaluationResult.getFloatValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE_FACTOR,
        intersectionSizeFactor);
    if (ok)
        icon->intersectionSizeFactor = intersectionSizeFactor;

    float intersectionSize = std::numeric_limits<float>::quiet_NaN();
    ok = !ok && primitive->evaluationResult.getFloatValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_SIZE,
        intersectionSize);
    if (ok)
        icon->intersectionSize = intersectionSize;

    float intersectionMargin = std::numeric_limits<float>::quiet_NaN();
    ok = !ok && primitive->evaluationResult.getFloatValue(
        env->styleBuiltinValueDefs->id_OUTPUT_INTERSECTION_MARGIN,
        intersectionMargin);
    if (ok)
        icon->intersectionMargin = intersectionMargin;

    //TODO: Obsolete
    if (!ok)
    {
        float iconIntersectionSize = std::numeric_limits<float>::quiet_NaN();
        ok = primitive->evaluationResult.getFloatValue(
            env->styleBuiltinValueDefs->id_OUTPUT_ICON_INTERSECTION_SIZE,
            iconIntersectionSize);
        if (ok)
            icon->intersectionSize = iconIntersectionSize;
    }

    // In case new icon symbol has a twin that was already added or has the same location, ignore this one
    const auto hasTwinOrSameLocation = std::any_of(outSymbols,
        [icon]
        (const std::shared_ptr<const Symbol>& otherSymbol) -> bool
        {
            if (const auto otherIcon = std::dynamic_pointer_cast<const IconSymbol>(otherSymbol))
            {
                return icon->hasSameContentAs(*otherIcon) || icon->location31 == otherIcon->location31;
            }

            return false;
        });
    if (hasTwinOrSameLocation)
    {
        if (metric)
        {
            metric->elapsedTimeForIconSymbolsProcessing += iconProcessingStopwatch.elapsed();
            metric->rejectedIconSymbols++;
        }

        return;
    }

    // Icons are always first
    outSymbols.prepend(qMove(icon));
    if (metric)
    {
        metric->elapsedTimeForIconSymbolsProcessing += iconProcessingStopwatch.elapsed();
        metric->obtainedIconSymbols++;
    }
}

OsmAnd::MapPrimitiviser_P::Context::Context(
    const std::shared_ptr<const MapPresentationEnvironment>& env_,
    const ZoomLevel zoom_)
    : env(env_)
    , zoom(zoom_)
{
    polygonAreaMinimalThreshold = env->getPolygonAreaMinimalThreshold(zoom);
    roadDensityZoomTile = env->getRoadDensityZoomTile(zoom);
    roadsDensityLimitPerTile = env->getRoadsDensityLimitPerTile(zoom);
    defaultSymbolPathSpacing = env->getDefaultSymbolPathSpacing();
    defaultBlockPathSpacing = env->getDefaultBlockPathSpacing();
}

template<bool calculateClosestWinding>
inline void OsmAnd::MapPrimitiviser_P::clipCoastlineForTile(int index, int size,
    const QList<std::shared_ptr<const MapObject>>& coastlines, PointI& center,
    const PointI& topLeft, const PointI& bottomRight, QVector<Coastline>* result, QVector<PointI>* finishPoints,
    const int64_t maxSqDistance, int64_t& minSqDistance, double& distance, bool& isReliable)
{
    const std::deque<int>& sequence = {index};
    clipCoastlinesForTile<calculateClosestWinding>(
        sequence, size > 0 ? size : coastlines[index]->points31.size(), coastlines, center,
        topLeft, bottomRight, result, finishPoints, maxSqDistance, minSqDistance, distance, isReliable);
};

template<bool calculateClosestWinding>
inline void OsmAnd::MapPrimitiviser_P::clipCoastlinesForTile(const std::deque<int>& sequence, int size,
    const QList<std::shared_ptr<const MapObject>>& coastlines, PointI& center,
    const PointI& topLeft, const PointI& bottomRight, QVector<Coastline>* result, QVector<PointI>* finishPoints,
    const int64_t maxSqDistance, int64_t& minSqDistance, double& distance, bool& isReliable)
{
    QVector<PointI> segment;
    segment.reserve(size);
    PointI prevPoint;
    PointI sm(INT32_MIN, INT32_MIN);
    int prevCode = 0;
    bool next = false;
    int64_t signedArea = 0;
    PointI filteredPoints[4];
    bool pointsReliable[4];
    int filteredPointCount = 0;
    int headPointsToSkip = 0;
    int pointsToProcess = size - 1;
    const auto& firstCoastline = coastlines[sequence.front()]->points31;
    const auto& lastCoastline = coastlines[sequence.back()]->points31;
    const auto& first = firstCoastline.front();
    const auto& last = lastCoastline.back();
    const bool isCycle = first == last;
    bool useInterPoint = false;
    bool skipClosestWinding = false;
    if (isCycle)
    {
        if (size < 4)
            return;
        if (size == 4)
        {
            if (Utilities::intCrossProduct2D(
                firstCoastline[1] - first, lastCoastline[lastCoastline.size() - 2] - first) >= 0)
                return;
            skipClosestWinding = true;
        }
        useInterPoint = size > 5 && size < 8;
        const auto lastSize = lastCoastline.size();
        if (lastSize > 2)
            filteredPoints[0] = lastCoastline[lastSize - 3];
        else
        {
            const auto& prevCoastline = coastlines[sequence[sequence.size() - 2]]->points31;
            filteredPoints[0] = prevCoastline[prevCoastline.size() - 2];
        }
        filteredPoints[1] = lastCoastline[lastSize - 2];
        pointsReliable[0] = true;
        pointsReliable[1] = true;
        filteredPointCount = 2;
        headPointsToSkip = 2;
    }
    else if (calculateClosestWinding)
    {
        const auto toFirst = first - center;
        const auto toLast = last - center;
        if ((static_cast<int64_t>(toFirst.x) * toFirst.x + static_cast<int64_t>(toFirst.y) * toFirst.y < maxSqDistance)
            || (static_cast<int64_t>(toLast.x) * toLast.x + static_cast<int64_t>(toLast.y) * toLast.y < maxSqDistance))
        {
            minSqDistance = -1;
            isReliable = false;
        }
    }

    bool oneIsProcessed = false;
    bool skipFirst = false;
    bool hasInputPoint = false;
    PointI previousInputPoint;
    const auto count = sequence.size();
    bool isExtra = false;
    for (int idx = 0; idx <= count; idx++)
    {
        if (idx == count)
        {
            isExtra = filteredPointCount > 1 && (!isCycle || oneIsProcessed
                || (filteredPointCount > 3 && filteredPoints[1] != filteredPoints[3])
                || (filteredPointCount > 2 && filteredPoints[0] != filteredPoints[2]));
            if (!isExtra)
                break;
            skipFirst = false;
            hasInputPoint = false;
        }
        const auto& coastline = isExtra
            ? QVector<PointI>(filteredPoints, filteredPoints + filteredPointCount)
            : coastlines[sequence[idx]]->points31;
        int j = 0;
        for (const auto& point : coastline)
        {
            if (Q_UNLIKELY(skipFirst))
            {
                skipFirst = false;
                continue;
            }
            if (Q_LIKELY(hasInputPoint))
            {
                if (point == previousInputPoint)
                {
                    pointsToProcess--;
                    j++;
                    continue;
                }
                bool shouldProcess = false;
                PointI startPoint, endPoint;
                bool startReliable = false;
                bool endReliable = false;
                if (isExtra)
                {
                    startPoint = previousInputPoint;
                    endPoint = point;
                    startReliable = pointsReliable[j++];
                    endReliable = pointsReliable[j];
                    shouldProcess = true;
                }
                else if (pointsToProcess-- > 0)
                {
                    // Remove short self-intersecting loops before clipping or measuring winding.
                    const auto interPoint = filteredPointCount > 2 ? Utilities::getIntersectionPoint(
                        filteredPoints[filteredPointCount - 3], filteredPoints[filteredPointCount - 2],
                        filteredPoints[filteredPointCount - 1], point, useInterPoint) : PointI(-1, -1);
                    if (interPoint.x >= 0)
                    {
                        if (isCycle && headPointsToSkip > 0)
                        {
                            if (filteredPointCount > 3)
                            {
                                pointsToProcess--;
                                headPointsToSkip--;
                            }
                            else
                            {
                                pointsToProcess -= headPointsToSkip;
                                headPointsToSkip = 0;
                            }
                        }
                        filteredPointCount -= 2;
                        pointsReliable[filteredPointCount - 1] = false;
                        if (useInterPoint)
                        {
                            filteredPoints[filteredPointCount] = interPoint;
                            pointsReliable[filteredPointCount++] = false;
                        }
                        if (point != filteredPoints[filteredPointCount - 1])
                        {
                            filteredPoints[filteredPointCount] = point;
                            pointsReliable[filteredPointCount++] = false;
                        }
                    }
                    else if (filteredPointCount > 3)
                    {
                        if ((!isCycle || size > 7) && Utilities::getIntersectionPoint(
                            filteredPoints[filteredPointCount - 4], filteredPoints[filteredPointCount - 3],
                            filteredPoints[filteredPointCount - 1], point, false).x >= 0)
                        {
                            if (isCycle && headPointsToSkip > 0)
                            {
                                pointsToProcess -= headPointsToSkip;
                                headPointsToSkip = 0;
                            }
                            filteredPointCount -= 3;
                            pointsReliable[filteredPointCount - 1] = false;
                            if (point != filteredPoints[filteredPointCount - 1])
                            {
                                filteredPoints[filteredPointCount] = point;
                                pointsReliable[filteredPointCount++] = false;
                            }
                        }
                        else
                        {
                            if (headPointsToSkip > 0)
                                headPointsToSkip--;
                            else
                            {
                                oneIsProcessed = true;
                                startPoint = filteredPoints[0];
                                endPoint = filteredPoints[1];
                                startReliable = pointsReliable[0];
                                endReliable = pointsReliable[1];
                                shouldProcess = true;
                            }
                            filteredPoints[0] = filteredPoints[1];
                            filteredPoints[1] = filteredPoints[2];
                            filteredPoints[2] = filteredPoints[3];
                            filteredPoints[3] = point;
                            pointsReliable[0] = pointsReliable[1];
                            pointsReliable[1] = pointsReliable[2];
                            pointsReliable[2] = pointsReliable[3];
                            pointsReliable[3] = true;
                        }
                    }
                    else
                    {
                        filteredPoints[filteredPointCount] = point;
                        pointsReliable[filteredPointCount++] = true;
                    }
                }
                if (shouldProcess)
                {
                    const auto nextCode = Utilities::computeOutCode(endPoint, topLeft, bottomRight);
                    auto p0 = startPoint;
                    auto p1 = endPoint;
                    int code0 = next && startPoint == prevPoint
                        ? prevCode : Utilities::computeOutCode(startPoint, topLeft, bottomRight);
                    int code1 = nextCode;
                    bool accept = false;
                    while (true)
                    {
                        if ((code0 | code1) == 0)
                        {
                            accept = true;
                            break;
                        }
                        else if ((code0 & code1) > 0)
                            break;
                        else if (code0 > 0)
                        {
                            p0 = Utilities::getIntersection(code0, p0, p1, topLeft, bottomRight);
                            code0 = Utilities::computeOutCode(p0, topLeft, bottomRight);
                        }
                        else
                        {
                            p1 = Utilities::getIntersection(code1, p0, p1, topLeft, bottomRight);
                            code1 = Utilities::computeOutCode(p1, topLeft, bottomRight);
                        }
                    }
                    const bool isEmpty = segment.empty();
                    if (accept && p0 != p1)
                    {
                        if (isEmpty)
                        {
                            sm.x = INT32_MIN;
                            sm.y = INT32_MIN;
                        }
                        if (isEmpty || segment.back() != p0)
                        {
                            if (!isEmpty)
                            {
                                const auto borderCode = Utilities::computeBorderCode(
                                    segment.front(), topLeft, bottomRight);
                                const auto lastBorderCode = Utilities::computeBorderCode(
                                    segment.back(), topLeft, bottomRight);
                                finishPoints[lastBorderCode].push_back(segment.back());
                                result[borderCode].push_back({qMove(segment), signedArea, INT32_MAX});
                                signedArea = 0;
                                segment.reserve(size);
                            }
                            if (p0 != sm)
                            {
                                segment.push_back(p0);
                                sm = p0;
                            }
                        }
                        if (p1 != sm)
                        {
                            signedArea += Utilities::intCrossProduct2D(p0, p1);
                            segment.push_back(p1);
                            sm = p1;
                        }
                    }
                    else if (!isEmpty)
                    {
                        if (segment.size() > 1)
                        {
                            const auto borderCode = Utilities::computeBorderCode(
                                segment.front(), topLeft, bottomRight);
                            const auto lastBorderCode = Utilities::computeBorderCode(
                                segment.back(), topLeft, bottomRight);
                            finishPoints[lastBorderCode].push_back(segment.back());
                            result[borderCode].push_back({qMove(segment), signedArea, INT32_MAX});
                        }
                        signedArea = 0;
                        segment.reserve(size);
                    }
                    prevPoint = endPoint;
                    prevCode = nextCode;
                    next = true;
                    if (calculateClosestWinding && !skipClosestWinding)
                    {
                        Utilities::findClosestWinding(center, startPoint, endPoint, startReliable, endReliable,
                            maxSqDistance, minSqDistance, distance, isReliable);
                    }
                }
            }
            else
            {
                hasInputPoint = true;
                if (!isExtra)
                {
                    filteredPoints[filteredPointCount] = point;
                    pointsReliable[filteredPointCount++] = true;
                }
            }
            previousInputPoint = point;
        }
        skipFirst = true;
    }
    if (segment.size() > 1)
    {
        const auto borderCode = Utilities::computeBorderCode(segment.front(), topLeft, bottomRight);
        const auto lastBorderCode = Utilities::computeBorderCode(segment.back(), topLeft, bottomRight);
        finishPoints[lastBorderCode].push_back(segment.back());
        result[borderCode].push_back({qMove(segment), signedArea, INT32_MAX});
    }
};

bool OsmAnd::MapPrimitiviser_P::getCoastlines(
    const AreaI area31,
    const AreaI64 coastlineArea31,
    const QList<std::shared_ptr<const MapObject>>& coastlines,
    QList<std::shared_ptr<const MapObject>>& outVectorized,
    MapSurfaceType& surfaceType,
    bool* brokenCoastlineFault /* = nullptr */)
{
    outVectorized.clear();
    const bool withSurfaceType = area31.left() != coastlineArea31.left();
    bool withCoastlines = false;
    QVector<int> polylineIndices;
    polylineIndices.reserve(coastlines.size());
    QHash<PointI, QPair<PointI, PointI>> heads;
    QHash<PointI, PointI> tails;
    QVector<std::deque<int>> sequences;
    QVector<int> sizes;
    const auto mask = static_cast<uint32_t>(-1) << 5;
    const PointI topLeft(area31.left() & mask, area31.top() & mask);
    const PointI bottomRight(
        area31.safeEnlarge(area31.right(), 31) & mask, area31.safeEnlarge(area31.bottom(), 31) & mask);
    int idx = 0;
    auto cend = coastlines.cend();
    for (auto it = coastlines.cbegin(); it != cend; it++, idx++)
    {
        const auto& coastline = *it;
        const auto& points = coastline->points31;
        const auto& front = points.front();
        const auto& back = points.back();
        const auto size = points.size();
        if (front != back)
        {
            // Gather segment sequences to use them when searching for possible intersections
            const auto& tailIt = tails.find(front);
            if (tailIt != tails.end())
            {
                const auto start = tailIt.value();
                const auto& headIt = heads.find(start);
                if (headIt != heads.end())
                {
                    auto value = headIt.value().first;
                    if (value.x < 0)
                    {
                        const auto i = -value.x - 1;
                        sequences[i].push_back(idx);
                        sizes[i] += size - 1;
                    }
                    else
                    {
                        sequences.push_back(qMove(std::deque<int>({value.x, idx})));
                        sizes.push_back(value.y + size - 1);
                        value.x = -sequences.size();
                        (*headIt).first.x = value.x;
                    }
                    tails.erase(tailIt);
                    const auto& hIt = heads.find(back);
                    if (hIt != heads.end() && hIt != headIt)
                    {
                        const auto& head = hIt.value();
                        const auto end = head.second;
                        const auto& tIt = tails.find(end);
                        if (tIt != tails.end())
                            *tIt = start;
                        (*headIt).second = end;
                        const auto v = head.first;
                        const auto s = -v.x - 1;
                        const auto d = -value.x - 1;
                        if (v.x < 0)
                        {
                            auto& src = sequences[s];
                            auto& dst = sequences[d];
                            dst.insert(dst.end(), src.begin(), src.end());
                            src.clear();
                            sizes[d] += sizes[s] - 1;
                        }
                        else
                        {
                            sequences[d].push_back(v.x);
                            sizes[d] += v.y - 1;
                        }
                        heads.erase(hIt);
                    }
                    else
                    {
                        (*headIt).second = back;
                        tails.insert(back, start);
                    }
                }
            }
            else
            {
                const auto& headIt = heads.find(back);
                if (headIt != heads.end())
                {
                    const auto& head = headIt.value();
                    const auto end = head.second;
                    const auto& tailIt = tails.find(end);
                    if (tailIt != tails.end())
                        *tailIt = front;
                    auto value = head.first;
                    if (value.x < 0)
                    {
                        const auto i = -value.x - 1;
                        sequences[i].push_front(idx);
                        sizes[i] += size - 1;
                    }
                    else
                    {
                        sequences.push_back(qMove(std::deque<int>({idx, value.x})));
                        sizes.push_back(value.y + size - 1);
                        value.x = -sequences.size();
                    }
                    heads.erase(headIt);
                    heads.insert(front, QPair<PointI, PointI>(value, end));
                }
                else
                {
                    heads.insert(front, QPair<PointI, PointI>(PointI(idx, size), back));
                    tails.insert(back, front);
                }
            }
            continue;
        }
        if (size < 4)
            continue;

        if (size == 4 && Utilities::intCrossProduct2D(points[1] - front, points[2] - front) >= 0)
            continue;

        // Process already polygonized big coastlines
        // (workaround "lake inside the continent")
        if (coastline->bbox31.left() < topLeft.x || coastline->bbox31.right() > bottomRight.x
            || coastline->bbox31.top() < topLeft.y || coastline->bbox31.bottom() > bottomRight.y)
        {
            polylineIndices.push_back(idx);
            continue;
        }

        // Get already polygonized coastline
        QVector<PointI> polygon;
        bool isClockwise;
        Utilities::calcPolygonInTile(points, polygon, isClockwise);
        const auto mapObject = std::make_shared<CoastlineMapObject>();
        mapObject->points31 = qMove(polygon);
        mapObject->isArea = coastline->isArea;
        mapObject->attributeIds.push_back(isClockwise
            ? MapObject::defaultAttributeMapping->naturalCoastlineAttributeId
            : MapObject::defaultAttributeMapping->naturalLandAttributeId);
        if (isClockwise)
            withCoastlines = true;
        outVectorized.push_back(mapObject);
    }

    if (!polylineIndices.isEmpty() || !heads.isEmpty())
    {
        PointI center;
        int64_t maxSqDistance = 0;
        int64_t minSqDistance = INT64_MAX;
        double distance = 0.0;
        bool isReliable = false;
        AreaI innerArea;
        if (withSurfaceType)
        {
            center = PointI(topLeft.x + (bottomRight.x - topLeft.x) / 2,
                topLeft.y + (bottomRight.y - topLeft.y) / 2);
            const auto radius = qMin(
                qMin(center.x - coastlineArea31.left(), coastlineArea31.right() - center.x),
                qMin(center.y - coastlineArea31.top(), coastlineArea31.bottom() - center.y)) & mask;
            maxSqDistance = radius * radius;
            innerArea.top() = static_cast<int>(qMax(coastlineArea31.top(), static_cast<int64_t>(0))) + 1;
            innerArea.left() = static_cast<int>(qMax(coastlineArea31.left(), static_cast<int64_t>(0))) + 1;
            innerArea.bottom() = static_cast<int>(qMin(coastlineArea31.bottom(), static_cast<int64_t>(INT32_MAX))) - 1;
            innerArea.right() = static_cast<int>(qMin(coastlineArea31.right(), static_cast<int64_t>(INT32_MAX))) - 1;
        }

        QVector<Coastline> coastlineGroups[5];
        QVector<PointI> finishPoints[5];

        for (int i : polylineIndices)
        {
            if (withSurfaceType)
                clipCoastlineForTile<true>(i, 0, coastlines, center, topLeft, bottomRight,
                    coastlineGroups, finishPoints, maxSqDistance, minSqDistance, distance, isReliable);
            else
                clipCoastlineForTile<false>(i, 0, coastlines, center, topLeft, bottomRight,
                    coastlineGroups, finishPoints, maxSqDistance, minSqDistance, distance, isReliable);
        }
        for (const auto& head : constOf(heads))
        {
            if (head.first.x >= 0)
            {
                if (withSurfaceType)
                {
                    const auto& first = coastlines[head.first.x]->points31.front();
                    const auto& last = coastlines[head.first.x]->points31.back();
                    if (brokenCoastlineFault && first != last
                        && (innerArea.contains(first.x, first.y) || innerArea.contains(last.x, last.y)))
                    {
                        *brokenCoastlineFault = true;
                        outVectorized.clear();
                        return false;
                    }
                    clipCoastlineForTile<true>(head.first.x, head.first.y, coastlines, center, topLeft, bottomRight,
                        coastlineGroups, finishPoints, maxSqDistance, minSqDistance, distance, isReliable);
                }
                else
                    clipCoastlineForTile<false>(head.first.x, head.first.y, coastlines, center, topLeft, bottomRight,
                        coastlineGroups, finishPoints, maxSqDistance, minSqDistance, distance, isReliable);
            }
            else
            {
                const auto i = -head.first.x - 1;
                if (withSurfaceType)
                {
                    const auto& first = coastlines[sequences[i].front()]->points31.front();
                    const auto& last = coastlines[sequences[i].back()]->points31.back();
                    if (brokenCoastlineFault && first != last
                        && (innerArea.contains(first.x, first.y) || innerArea.contains(last.x, last.y)))
                    {
                        *brokenCoastlineFault = true;
                        outVectorized.clear();
                        return false;
                    }
                    clipCoastlinesForTile<true>(sequences[i], sizes[i], coastlines, center, topLeft, bottomRight,
                        coastlineGroups, finishPoints, maxSqDistance, minSqDistance, distance, isReliable);
                }
                else
                    clipCoastlinesForTile<false>(sequences[i], sizes[i], coastlines, center, topLeft, bottomRight,
                        coastlineGroups, finishPoints, maxSqDistance, minSqDistance, distance, isReliable);
            }
        }

        // Find possible crossed coastline and prepare to start a polygon from it
        int minProximity = INT32_MAX;
        int fromSide = 0;
        int fromIndex = -1;
        for (int side = 0; side < 4; side++)
        {
            int i = 0;
            for (auto& coastline : coastlineGroups[side])
            {
                const auto& startPoint = coastline.points.front();
                for (const auto& finishPoint : finishPoints[side])
                {
                    const auto proximity = abs(Utilities::getOffsetOnBorder(finishPoint, startPoint, side));
                    if (proximity < coastline.proximity)
                        coastline.proximity = proximity;
                }
                if (coastline.proximity > 0 && coastline.proximity < minProximity)
                {
                    minProximity = coastline.proximity;
                    fromSide = side;
                    fromIndex = i;
                }
                i++;
            }
        }
        if (fromIndex >= 0)
        {
            const auto lastIdx = coastlineGroups[fromSide].size() - 1;
            if (fromIndex < lastIdx)
                coastlineGroups[fromSide].swapItemsAt(fromIndex, lastIdx);
        }

        // Combine polylines into polygons
        QVector<Span> previousSpans;
        QVector<Span> polygonSpans;
        previousSpans.reserve(10);
        polygonSpans.reserve(10);
        bool checkOverlap = true;
        const auto tillSide = fromSide + 4;
        for (int border = fromSide; border <= tillSide; border++)
        {
            bool fromInside = border == tillSide;
            const auto beginSide = fromInside ? 4 : border % 4;
            auto& polylines = coastlineGroups[beginSide];
            while (!polylines.empty())
            {
                if (checkOverlap)
                    polygonSpans.clear();
                auto polyline = polylines.takeLast();
                auto polygon = qMove(polyline.points);
                auto winding = polyline.winding;
                auto windingFull = winding;
                auto startPoint = polygon.front();
                auto startSide = beginSide;
                bool checkOne = true;
                auto startPointFull = startPoint;
                auto startSideFull = beginSide;
                bool checkFull = true;
                auto lastPoint = polygon.back();
                auto endSide = Utilities::computeBorderCode(lastPoint, topLeft, bottomRight);
                if (fromInside && endSide != 4)
                {
                    if (brokenCoastlineFault)
                    {
                        *brokenCoastlineFault = true;
                        outVectorized.clear();
                        return false;
                    }
                    continue;
                }
                auto finishPoint = lastPoint;
                auto finishSide = endSide;
                int prevSide;
                int i = -2;
                int lastSide = 0;
                bool isComplete = false;
                bool withExtraCycle = false;
                while (++i <= lastSide)
                {
                    if (endSide != 4 && i < 0)
                    {
                        i = endSide;
                        lastSide = beginSide + (beginSide <= endSide ? 4 : 0);
                    }
                    while (endSide == 4)
                    {
                        auto& nextPolylines = coastlineGroups[endSide];
                        int segIdx;
                        const auto segCount = nextPolylines.size();
                        for (segIdx = 0; segIdx < segCount; segIdx++)
                        {
                            if (nextPolylines[segIdx].points.front() == lastPoint)
                            {
                                const auto lastIdx = nextPolylines.size() - 1;
                                if (segIdx < lastIdx)
                                    nextPolylines.swapItemsAt(segIdx, lastIdx);
                                const auto nextPolyline = nextPolylines.takeLast();
                                winding += nextPolyline.winding;
                                windingFull += nextPolyline.winding;
                                polygon.pop_back();
                                polygon.append(nextPolyline.points);
                                break;
                            }
                        }
                        if (segIdx == segCount)
                            break;
                        lastPoint = polygon.back();
                        const auto nextSide = Utilities::computeBorderCode(lastPoint, topLeft, bottomRight);
                        if (nextSide < 4)
                        {
                            if (i < 0)
                            {
                                i = nextSide;
                                lastSide = beginSide + (beginSide <= nextSide ? 4 : 0);
                            }
                            else
                                i += (nextSide < prevSide ? 4 : 0) + nextSide - prevSide;
                        }
                        endSide = nextSide;
                        finishPoint = lastPoint;
                        finishSide = endSide;
                    }
                    if (fromInside)
                    {
                        isComplete = endSide == 4 && polygon.size() > 3 && polygon.back() == startPointFull;
                        break;
                    }
                    if (endSide == 4)
                        break;
                    const auto side = i % 4;
                    auto testPoint = lastPoint;
                    if (checkOne && side == finishSide && startSide == finishSide)
                    {
                        checkOne = false;
                        bool isWater = winding + Utilities::intCrossProduct2D(finishPoint, startPoint) >= 0;
                        const auto offset = Utilities::getOffsetOnBorder(finishPoint, startPoint, side);
                        if (isWater && offset > 0)
                        {
                            testPoint = startPoint;
                            finishPoint = testPoint;
                        }
                        else if (!isWater && offset <= 0)
                        {
                            testPoint = startPoint;
                            if ((side & 1) > 0)
                                testPoint.y += side > 1 ? -1 : 1;
                            else
                                testPoint.x += side > 1 ? -1 : 1;
                            finishPoint = testPoint;
                        }
                    }
                    if (checkFull && side == endSide && startSideFull == endSide)
                    {
                        checkFull = false;
                        bool isWater = windingFull + Utilities::intCrossProduct2D(lastPoint, startPointFull) >= 0;
                        const auto offset = Utilities::getOffsetOnBorder(lastPoint, startPointFull, side);
                        if (isWater && offset > 0)
                        {
                            testPoint = startPointFull;
                            finishPoint = testPoint;
                        }
                        else if (!isWater && offset <= 0)
                        {
                            testPoint = startPointFull;
                            if ((side & 1) > 0)
                                testPoint.y += side > 1 ? -1 : 1;
                            else
                                testPoint.x += side > 1 ? -1 : 1;
                            finishPoint = testPoint;
                        }
                    }
                    auto& nextPolylines = coastlineGroups[side];
                    const auto segCount = nextPolylines.size();
                    int minDistance = INT32_MAX;
                    int minIdx = -1;
                    int segTotal = side == beginSide ? segCount + 1 : segCount;
                    for (int segIdx = 0; segIdx < segTotal; segIdx++)
                    {
                        const auto& p = segIdx < segCount ? nextPolylines[segIdx].points.front() : startPointFull;
                        const auto offset = Utilities::getOffsetOnBorder(p, testPoint, side);
                        if (!withExtraCycle && offset == 0 && testPoint == lastPoint)
                        {
                            lastSide += 4;
                            withExtraCycle = true;
                        }
                        if (offset >= 0 && offset < minDistance)
                        {
                            minDistance = offset;
                            minIdx = segIdx;
                        }
                    }
                    if (minIdx >= 0)
                    {
                        double finishOffset, startOffset;
                        if (checkOverlap)
                            finishOffset =
                                Utilities::getOffsetOnPerimeter(finishPoint, finishSide, topLeft, bottomRight);
                        if (minIdx < segCount)
                        {
                            const auto lastIdx = nextPolylines.size() - 1;
                            if (minIdx < lastIdx)
                                nextPolylines.swapItemsAt(minIdx, lastIdx);
                            const auto nextPolyline = nextPolylines.takeLast();
                            winding = nextPolyline.winding;
                            startPoint = nextPolyline.points.front();
                            windingFull += Utilities::intCrossProduct2D(lastPoint, startPoint) + winding;
                            startSide = Utilities::computeBorderCode(startPoint, topLeft, bottomRight);
                            checkOne = true;
                            if (checkOverlap)
                            {
                                startOffset =
                                    Utilities::getOffsetOnPerimeter(startPoint, startSide, topLeft, bottomRight);
                                if (checkOverlap && startOffset != finishOffset)
                                    polygonSpans.push_back({finishOffset,
                                        startOffset < finishOffset ? startOffset + 4.0 : startOffset, 0, -1});
                            }
                            if (nextPolyline.points.front() == lastPoint)
                                polygon.pop_back();
                            polygon.append(nextPolyline.points);
                            lastPoint = polygon.back();
                            const auto nextSide = Utilities::computeBorderCode(lastPoint, topLeft, bottomRight);
                            if (nextSide < 4)
                                i += (nextSide < endSide ? 4 : 0) + nextSide - endSide - 1;
                            else
                            {
                                prevSide = endSide;
                                i -= 1;
                            }
                            endSide = nextSide;
                            finishPoint = lastPoint;
                            finishSide = endSide;
                            continue;
                        }
                        else if (checkOverlap)
                        {
                            startOffset =
                                Utilities::getOffsetOnPerimeter(startPointFull, startSideFull, topLeft, bottomRight);
                            if (checkOverlap && startOffset != finishOffset)
                                polygonSpans.push_back({finishOffset,
                                    startOffset < finishOffset ? startOffset + 4.0 : startOffset, 0, -1});
                        }
                        isComplete = true;
                        break;
                    }
                    if (i < lastSide)
                    {
                        const auto previousPoint = lastPoint;
                        lastPoint.x = side > 1 ? topLeft.x : bottomRight.x;
                        lastPoint.y = side > 0 && side < 3 ? bottomRight.y : topLeft.y;
                        polygon.push_back(lastPoint);
                        endSide = (endSide + 1) % 4;
                        windingFull += Utilities::intCrossProduct2D(previousPoint, lastPoint);
                    }
                }
                if (!isComplete)
                {
                    if (brokenCoastlineFault)
                    {
                        *brokenCoastlineFault = true;
                        outVectorized.clear();
                        return false;
                    }
                    continue;
                }
                if (fromInside)
                {
                    const bool isClockwise = winding >= 0;
                    const auto mapObject = std::make_shared<CoastlineMapObject>();
                    mapObject->points31 = qMove(polygon);
                    mapObject->isArea = true;
                    mapObject->attributeIds.push_back(isClockwise
                        ? MapObject::defaultAttributeMapping->naturalCoastlineAttributeId
                        : MapObject::defaultAttributeMapping->naturalLandAttributeId);
                    if (isClockwise)
                        withCoastlines = true;
                    outVectorized.push_back(mapObject);
                    continue;
                }
                if (polygon.size() < 3)
                    continue;
                const auto& first = startPointFull;
                if (first != polygon.back())
                    polygon.push_back(first);
                if (polygon.size() < 4)
                    continue;

                const auto count = outVectorized.size();
                int64_t polygonArea;
                auto polygonIndex = count;
                bool replacePolygon = false;
                if (checkOverlap)
                {
                    polygonArea = windingFull + Utilities::intCrossProduct2D(lastPoint, startPointFull);
                    for (const auto& previousSpan : constOf(previousSpans))
                    {
                        const auto fromOffset = previousSpan.fromOffset;
                        const auto toOffset = previousSpan.toOffset;
                        const bool prevFar = toOffset >= 4.0;
                        for (const auto& span : constOf(polygonSpans))
                        {
                            const bool spanFar = span.toOffset >= 4.0;
                            const bool outside = !(prevFar && spanFar)
                                && ((prevFar && span.fromOffset + 4.0 >= toOffset && span.toOffset <= fromOffset)
                                || (spanFar && fromOffset + 4.0 >= span.toOffset && toOffset <= span.fromOffset)
                                || (!prevFar && !spanFar
                                    && (span.fromOffset >= toOffset || fromOffset >= span.toOffset)));
                            if (!outside)
                            {
                                checkOverlap = false;
                                polygonIndex = previousSpan.index;
                                replacePolygon = polygonArea < previousSpan.area;
                                break;
                            }
                        }
                        if (!checkOverlap)
                            break;
                    }
                }

                withCoastlines = true;

                const auto mapObject = std::make_shared<CoastlineMapObject>();
                mapObject->points31 = qMove(polygon);
                mapObject->isArea = true;
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineAttributeId);
                if (polygonIndex == count)
                    outVectorized.push_back(mapObject);
                else if (replacePolygon)
                    outVectorized[polygonIndex] = mapObject;
                if (checkOverlap)
                {
                    for (auto& span : polygonSpans)
                    {
                        span.area = polygonArea;
                        span.index = polygonIndex;
                    }
                    previousSpans.append(polygonSpans);
                }
            }
        }
        if (withSurfaceType && isReliable && minSqDistance < maxSqDistance && distance != 0.0)
            surfaceType = distance < 0.0 ? MapSurfaceType::FullLand : MapSurfaceType::FullWater;
    }

    if (outVectorized.isEmpty())
        return false;

    if (!withCoastlines)
    {
        QVector<PointI> polygon;
        polygon.reserve(5);
        polygon.push_back(topLeft);
        polygon.push_back(PointI(bottomRight.x, topLeft.y));
        polygon.push_back(bottomRight);
        polygon.push_back(PointI(topLeft.x, bottomRight.y));
        polygon.push_back(topLeft);
        const auto mapObject = std::make_shared<CoastlineMapObject>();
        mapObject->points31 = qMove(polygon);
        mapObject->isArea = true;
        mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineAttributeId);
        outVectorized.push_back(mapObject);
    }

    return true;
}

bool OsmAnd::MapPrimitiviser_P::polygonizeCoastlines(
    const AreaI area31,
    const ZoomLevel zoom,
    const QList< std::shared_ptr<const MapObject> >& coastlines,
    QList< std::shared_ptr<const MapObject> >& outVectorized)
{
    std::vector<FoundMapDataObject> legacyCoastlineObjects;
    QList<shared_ptr<MapDataObject>> legacyObjectsHolder;
    for (const auto & c : constOf(coastlines))
    {
        auto dataObj = std::make_shared<MapDataObject>(convertToLegacy(*c));
        legacyObjectsHolder.push_back(dataObj);
        FoundMapDataObject legacyObj(dataObj.get());
        legacyCoastlineObjects.push_back(legacyObj);
    }
    std::vector<FoundMapDataObject> legacyCoastlineObjectsResult;
    // Get polygons of coastlines circumcised by sides of area31 or isolated islands/lakes
    bool processed = processCoastlines(legacyCoastlineObjects, (int) area31.topLeft.x, (int) area31.bottomRight.x, (int) area31.bottomRight.y,
                      (int) area31.topLeft.y, static_cast<int>(zoom), false, false, legacyCoastlineObjectsResult);
    if (processed)
    {
        int size = 0;
        for (const auto & legacyCoastline : constOf(legacyCoastlineObjectsResult))
        {
            const auto & mapObj = convertFromLegacy(legacyCoastline.obj);
            size += mapObj->points31.size();
            outVectorized.push_back(mapObj);
        }
        if (size == 3)
        {
            PointI prev;
            for (const auto & m : outVectorized)
            {
                for (const PointI & p : m->points31)
                {
                    if (prev == p)
                    {
                        return false;
                    }
                    prev = p;
                }
            }
        }
        if (size < 3)
        {
            // Polygon needs mimimum 3 points (bug with part of coastline on the border of tile)
            processed = false;
        }
    }
    return processed;
}

MapDataObject OsmAnd::MapPrimitiviser_P::convertToLegacy(const MapObject & coreObj)
{
    MapDataObject legacyObj;
    for (const auto & p : coreObj.points31)
    {
        legacyObj.points.push_back(std::make_pair(p.x, p.y));
    }
    for (const uint32_t & a : coreObj.attributeIds)
    {
        auto tagVal = coreObj.attributeMapping->decodeMap[a];
        legacyObj.types.push_back(std::make_pair(tagVal.tag.toStdString(), tagVal.value.toStdString()));
    }
    for (const uint32_t & a : coreObj.additionalAttributeIds)
    {
        auto tagVal = coreObj.attributeMapping->decodeMap[a];
        legacyObj.additionalTypes.push_back(std::make_pair(tagVal.tag.toStdString(), tagVal.value.toStdString()));
    }
    legacyObj.polygonInnerCoordinates.reserve(coreObj.innerPolygonsPoints31.size());
    for (const auto & innerVector : coreObj.innerPolygonsPoints31)
    {
        std::vector<pair<int, int> > coordinates;
        for (const auto & c : innerVector)
        {
            coordinates.push_back(std::make_pair(c.x, c.y));
        }
        legacyObj.polygonInnerCoordinates.push_back(coordinates);
    }
    legacyObj.area = coreObj.isArea;
    return legacyObj;
}

const std::shared_ptr<OsmAnd::MapObject> OsmAnd::MapPrimitiviser_P::convertFromLegacy(const MapDataObject * legacyObj)
{
    const std::shared_ptr<MapObject> mapObject(new CoastlineMapObject());
    for (auto const & point : constOf(legacyObj->points))
    {
        PointI p(point.first, point.second);
        mapObject->points31.push_back(p);
    }
    mapObject->isArea = legacyObj->area;
    for (const auto & t : constOf(legacyObj->types))
    {
        if (t.first == "natural")
        {
            if (t.second == "coastline")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineAttributeId);
            if (t.second == "land")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalLandAttributeId);
            if (t.second == "coastline_line")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineLineAttributeId);
            if (t.second == "coastline_broken")
                mapObject->attributeIds.push_back(MapObject::defaultAttributeMapping->naturalCoastlineBrokenAttributeId);
        }
    }
    return mapObject;
}


// Try to draw lines through of the center of area and find intersection with closest coastline
OsmAnd::MapSurfaceType OsmAnd::MapPrimitiviser_P::determineSurfaceType(AreaI area31, QList< std::shared_ptr<const MapObject> >& coastlines)
{
    const PointI center = area31.center();
    int minDist = INT_MAX;
    std::pair<PointI, PointI> foundSegment;
    bool isOcean = false;
    OsmAnd::MapSurfaceType surfaceType = MapSurfaceType::Undefined;
    for (const auto & mapObj : constOf(coastlines))
    {
        if (mapObj->points31.size() < 2)
            continue;
        
        for (int i = 0; i < mapObj->points31.size() - 1; i++)
        {
            const PointI & start = mapObj->points31.at(i);
            const PointI & end = mapObj->points31.at(i + 1);
            int intersectX = ray_intersect_x(start.x, start.y, end.x, end.y, center.y);
            if (intersectX != INT_MAX && intersectX != INT_MIN)
            {
                if (abs(intersectX - center.x) < minDist)
                {
                    // previous segment further than found
                    minDist = abs(intersectX - center.x);
                    
                    //water is always in right side
                    if ((intersectX < center.x && end.x > center.x) || (intersectX > center.x && end.x < center.x))
                    {
                        //the excluding when coastline is too big and "end" point in other quater
                        isOcean = (start.y - center.y > 0 && start.x - center.x < 0) || (start.y - center.y < 0 && start.x - center.x > 0);
                    }
                    else
                    {
                        //(end.y - center.y) * (end.x - center.x) - overflow INT
                        isOcean = (end.y - center.y < 0 && end.x - center.x < 0) || (end.y - center.y > 0 && end.x - center.x > 0);
                    }
                    
                    if (isOcean)
                    {
                        surfaceType = MapSurfaceType::FullWater;
                    }
                    else
                    {
                        surfaceType = MapSurfaceType::FullLand;
                    }
                }
            }
            int intersectY = ray_intersect_y(start.x, start.y, end.x, end.y, center.x);
            if (intersectY != INT_MAX && intersectY != INT_MIN)
            {
                if (abs(intersectY - center.y) < minDist)
                {
                    // previous segment further than found
                    minDist = abs(intersectY - center.y);
                    
                    //water is always in right side
                    if ((intersectY < center.y && end.y > center.y) || (intersectY > center.y && end.y < center.y))
                    {
                        //the excluding when coastline is too big and "end" point in other quater
                        isOcean =
                            (start.x > center.x && start.y > center.y) || (start.x < center.x && start.y < center.y);
                    }
                    else
                    {
                        isOcean = (end.x < center.x && end.y > center.y) || (end.x > center.x && end.y < center.y);
                    }
                    
                    if (isOcean)
                    {
                        surfaceType = MapSurfaceType::FullWater;
                    }
                    else
                    {
                        surfaceType = MapSurfaceType::FullLand;
                    }
                }
            }
        }
    }
    return surfaceType;
}

// Increase area on few points for avoid lines on the borders of tile
const OsmAnd::AreaI OsmAnd::MapPrimitiviser_P::getWidenArea(const AreaI & area31, const ZoomLevel & zoom) const
{
    if (zoom > ZoomLevel::ZoomLevel14)
    {
        int widen = zoom - ZoomLevel14;
        const AreaI & area = area31.getEnlargedBy(widen);
        return area;
    }
    return area31;
}

void OsmAnd::MapPrimitiviser_P::debugCoastline(const AreaI & area31, const QList< std::shared_ptr<const MapObject>> & coastlines) const
{
    LogPrintf(LogSeverityLevel::Debug, "area \n topLeft: [%d %d] [%f %f] \n bottomRight: [%d %d] [%f %f]",
              area31.top(), area31.left(),
              Utilities::get31LatitudeY(area31.topLeft.y),
              Utilities::get31LongitudeX(area31.topLeft.x),
              area31.bottom(), area31.right(),
              Utilities::get31LatitudeY(area31.bottomRight.y),
              Utilities::get31LongitudeX(area31.bottomRight.x));
    
    for (auto & c : coastlines)
    {
        LogPrintf(LogSeverityLevel::Debug, "LATITUDE, LONGITUDE");
        for (auto & p : c->points31)
        {
            LogPrintf(LogSeverityLevel::Debug, "%f, %f", Utilities::get31LatitudeY(p.y), Utilities::get31LongitudeX(p.x));
        }
    }
}


// ---------------- zoom-bench K: disk cache of primitivised tiles ----------------
#include <QDataStream>
#include "MapStyleValueDefinition.h"
#include <cstring>
#include "ObfMapSectionInfo.h"

namespace
{
    inline void zbWriteDouble(QDataStream& s, const double v)
    {
        quint64 bits;
        memcpy(&bits, &v, sizeof(bits));
        s << bits;
    }
    inline double zbReadDouble(QDataStream& s)
    {
        quint64 bits;
        s >> bits;
        double v;
        memcpy(&v, &bits, sizeof(v));
        return v;
    }
    void zbWritePoints(QDataStream& s, const QVector<OsmAnd::PointI>& pts)
    {
        s << static_cast<quint32>(pts.size());
        for (const auto& pt : pts)
            s << static_cast<qint32>(pt.x) << static_cast<qint32>(pt.y);
    }
    void zbReadPoints(QDataStream& s, QVector<OsmAnd::PointI>& pts)
    {
        quint32 n;
        s >> n;
        pts.resize(n);
        auto p = pts.data();
        for (quint32 i = 0; i < n; i++, p++)
        {
            qint32 x, y;
            s >> x >> y;
            p->x = x;
            p->y = y;
        }
    }
    const quint32 kZbMagic = 0x5A425033;
}

QByteArray OsmAnd::MapPrimitiviser_P::zbSerialize(
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& prim,
    const QList< std::shared_ptr<const MapObject> >& tileObjects,
    const MapSurfaceType surfaceType,
    const bool symbolsOnly,
    int* outDroppedGroups)
{
    QByteArray out;
    QDataStream s(&out, QIODevice::WriteOnly);
    s.setVersion(QDataStream::Qt_5_6);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QHash<const MapObject*, quint32> objIndex;
    QList< std::shared_ptr<const MapObject> > objs;
    const auto addObj =
        [&objIndex, &objs](const std::shared_ptr<const MapObject>& o) -> quint32
        {
            const auto it = objIndex.constFind(o.get());
            if (it != objIndex.cend())
                return *it;
            const quint32 i = objs.size();
            objIndex.insert(o.get(), i);
            objs.push_back(o);
            return i;
        };
    // zoom-bench M: labels only, prefiltered by guaranteed overlaps
    QSet<const MapObject*> keptGroups, keepObj, pathObjs;
    QSet<const MapPrimitiviser::Symbol*> zbKeepSym;
    if (symbolsOnly)
    {
        // E rules: best point caption per 8x8 cell of the tile, 2 best on-path captions per text; icons always
        const int shift = qMax(0, 31 - static_cast<int>(prim->zoom) - 3);
        QHash<quint64, QPair<int, const MapPrimitiviser::Symbol*>> bestInCell;
        QHash<QString, QVector<QPair<int, const MapPrimitiviser::Symbol*>>> onPathByText;
        for (auto it = prim->symbolsGroups.cbegin(); it != prim->symbolsGroups.cend(); ++it)
            for (const auto& sym : it.value()->symbols)
            {
                const auto text = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(sym);
                if (!text)
                {
                    zbKeepSym.insert(sym.get());
                    continue;
                }
                if (!text->drawOnPath)
                {
                    const quint64 cell = (static_cast<quint64>(static_cast<quint32>(text->location31.x >> shift)) << 32)
                        | static_cast<quint32>(text->location31.y >> shift);
                    const auto c = bestInCell.find(cell);
                    if (c == bestInCell.end() || text->order < c->first)
                        bestInCell[cell] = qMakePair(text->order, static_cast<const MapPrimitiviser::Symbol*>(sym.get()));
                }
                else
                    onPathByText[text->value].push_back(qMakePair(text->order, static_cast<const MapPrimitiviser::Symbol*>(sym.get())));
            }
        for (const auto& v : bestInCell)
            zbKeepSym.insert(v.second);
        for (auto it = onPathByText.begin(); it != onPathByText.end(); ++it)
        {
            auto& list = it.value();
            std::sort(list.begin(), list.end(),
                [](const QPair<int, const MapPrimitiviser::Symbol*>& l, const QPair<int, const MapPrimitiviser::Symbol*>& r)
                { return l.first < r.first; });
            for (int i = 0; i < list.size() && i < 2; i++)
                zbKeepSym.insert(list[i].second);
        }

        struct Cand
        {
            std::shared_ptr<const MapObject> key, src;
            double x, y, h;
            int order;
            QSet<QString> classes;
            bool path;
        };
        QVector<Cand> cands;
        const auto density = prim->mapPresentationEnvironment->displayDensityFactor;
        for (auto it = prim->symbolsGroups.cbegin(); it != prim->symbolsGroups.cend(); ++it)
        {
            const auto& sg = it.value();
            if (sg->symbols.isEmpty())
                continue;
            Cand c;
            c.key = it.key();
            c.src = sg->sourceObject;
            c.path = false;
            c.h = 0;
            c.order = std::numeric_limits<int>::max();
            const MapPrimitiviser::Symbol* firstKept = nullptr;
            for (const auto& sym : sg->symbols)
            {
                if (!zbKeepSym.contains(sym.get()))
                    continue;
                if (!firstKept)
                    firstKept = sym.get();
                const auto text = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(sym);
                if (sym->drawAlongPath || (text && text->drawOnPath))
                    c.path = true;
                c.h = qMax(c.h, text ? static_cast<double>(text->size) : 20.0 * density);
                c.order = qMin(c.order, sym->order);
                c.classes.unite(sym->intersectsWith);
            }
            if (!firstKept)
                continue;
            c.x = firstKept->location31.x / prim->scaleDivisor31ToPixel.x;
            c.y = firstKept->location31.y / prim->scaleDivisor31ToPixel.y;
            cands.push_back(c);
        }
        std::stable_sort(cands.begin(), cands.end(), [](const Cand& l, const Cand& r) { return l.order < r.order; });
        QVector<int> kept;
        int dropped = 0;
        for (int i = 0; i < cands.size(); i++)
        {
            const auto& c = cands[i];
            bool drop = false;
            if (!c.path && !c.classes.isEmpty())
            {
                for (const int j : kept)
                {
                    const auto& k = cands[j];
                    if (k.path || !k.classes.intersects(c.classes))
                        continue;
                    const double lim = (c.h + k.h) / 4.0;
                    const double dx = c.x - k.x, dy = c.y - k.y;
                    if (dx * dx + dy * dy < lim * lim)
                    {
                        drop = true;
                        break;
                    }
                }
            }
            if (drop)
            {
                dropped++;
                continue;
            }
            kept.push_back(i);
            keptGroups.insert(c.key.get());
            keepObj.insert(c.key.get());
            keepObj.insert(c.src.get());
            if (c.path)
            {
                pathObjs.insert(c.key.get());
                pathObjs.insert(c.src.get());
            }
        }
        if (outDroppedGroups)
            *outDroppedGroups = prim->symbolsGroups.size() - keptGroups.size();
    }
    MapPrimitiviser::PrimitivesGroupsCollection groups;
    for (const auto& g : prim->primitivesGroups)
        if (!symbolsOnly || keepObj.contains(g->sourceObject.get()))
            groups.push_back(g);

    if (!symbolsOnly)
        for (const auto& o : tileObjects)
            addObj(o);
    for (const auto& g : groups)
        addObj(g->sourceObject);
    for (auto it = prim->symbolsGroups.cbegin(); it != prim->symbolsGroups.cend(); ++it)
    {
        if (symbolsOnly && !keptGroups.contains(it.key().get()))
            continue;
        addObj(it.key());
        addObj(it.value()->sourceObject);
    }

    QHash<const MapPrimitiviser::Primitive*, quint64> primRef;
    for (int gi = 0; gi < groups.size(); gi++)
    {
        const auto& g = groups[gi];
        const MapPrimitiviser::PrimitivesCollection* lists[3] = { &g->polygons, &g->polylines, &g->points };
        for (quint64 k = 0; k < 3; k++)
            for (int i = 0; i < lists[k]->size(); i++)
                primRef.insert((*lists[k])[i].get(), (static_cast<quint64>(gi) << 34) | (k << 32) | static_cast<quint64>(i));
    }

    s << kZbMagic << static_cast<qint32>(prim->zoom) << static_cast<qint32>(surfaceType) << prim->coastlineMistake;
    zbWriteDouble(s, prim->scaleDivisor31ToPixel.x);
    zbWriteDouble(s, prim->scaleDivisor31ToPixel.y);

    s << static_cast<quint32>(objs.size()) << static_cast<quint32>(symbolsOnly ? 0 : tileObjects.size());
    for (const auto& o : objs)
    {
        qint32 kind = 3;
        const auto b = std::dynamic_pointer_cast<const BinaryMapObject>(o);
        if (b)
            kind = 0;
        else if (std::dynamic_pointer_cast<const MapPrimitiviser::SurfaceMapObject>(o))
            kind = 1;
        else if (std::dynamic_pointer_cast<const MapPrimitiviser::CoastlineMapObject>(o))
            kind = 2;
        if (kind == 3)
            return QByteArray();  // unknown object kind: do not cache this tile
        s << kind;
        if (b)
        {
            s << b->section->name << static_cast<qint32>(b->level->minZoom) << static_cast<qint32>(b->level->maxZoom)
              << static_cast<quint64>(b->id.id) << static_cast<quint64>(b->blockId.id);
        }
        s << o->attributeIds << o->additionalAttributeIds << o->isArea << o->isCoastline;
        if (symbolsOnly && !pathObjs.contains(o.get()))
        {
            zbWritePoints(s, QVector<PointI>());
            s << static_cast<quint32>(0);
        }
        else
        {
            zbWritePoints(s, o->points31);
            s << static_cast<quint32>(o->innerPolygonsPoints31.size());
            for (const auto& inner : o->innerPolygonsPoints31)
                zbWritePoints(s, inner);
        }
        s << static_cast<qint32>(o->bbox31.top()) << static_cast<qint32>(o->bbox31.left())
          << static_cast<qint32>(o->bbox31.bottom()) << static_cast<qint32>(o->bbox31.right());
        s << o->captions << o->captionsOrder << static_cast<qint32>(o->labelX) << static_cast<qint32>(o->labelY);
    }

    // value definition ids differ between processes: write a table of their names first
    QHash<qint32, qint32> defIndex;
    QStringList defNames;
    for (const auto& g : groups)
        for (const auto* list : { &g->polygons, &g->polylines, &g->points })
            for (const auto& pr : *list)
                for (const auto& e : pr->evaluationResult.entries)
                    if (!defIndex.contains(e.first))
                    {
                        const auto& def = prim->mapPresentationEnvironment->mapStyle->getValueDefinitionRefById(e.first);
                        if (!def)
                            return QByteArray();
                        defIndex.insert(e.first, defNames.size());
                        defNames.push_back(def->name);
                    }
    s << defNames;
    const auto writePrimitive =
        [&s, &defIndex](const std::shared_ptr<const MapPrimitiviser::Primitive>& pr)
        {
            s << static_cast<quint32>(pr->type) << static_cast<quint32>(pr->attributeIdIndex) << static_cast<qint32>(pr->zOrder)
              << static_cast<qint64>(pr->doubledArea) << pr->detailScaleFactor;
            s << static_cast<quint32>(pr->evaluationResult.entries.size());
            for (const auto& e : pr->evaluationResult.entries)
                s << static_cast<qint32>(defIndex.value(e.first)) << e.second;
        };
    s << static_cast<quint32>(groups.size());
    for (const auto& g : groups)
    {
        s << objIndex.value(g->sourceObject.get());
        for (const auto* list : { &g->polygons, &g->polylines, &g->points })
        {
            s << static_cast<quint32>(list->size());
            for (const auto& pr : *list)
                writePrimitive(pr);
        }
    }
    for (const auto* list : { &prim->polygons, &prim->polylines, &prim->points })
    {
        if (symbolsOnly)
        {
            s << static_cast<quint32>(0);
            continue;
        }
        s << static_cast<quint32>(list->size());
        for (const auto& pr : *list)
        {
            const auto it = primRef.constFind(pr.get());
            if (it == primRef.cend())
                return QByteArray();
            s << *it;
        }
    }

    s << static_cast<quint32>(symbolsOnly ? keptGroups.size() : prim->symbolsGroups.size());
    for (auto it = prim->symbolsGroups.cbegin(); it != prim->symbolsGroups.cend(); ++it)
    {
        if (symbolsOnly && !keptGroups.contains(it.key().get()))
            continue;
        const auto& sg = it.value();
        s << objIndex.value(it.key().get()) << objIndex.value(sg->sourceObject.get()) << sg->canBeShownWithoutIcon;
        quint32 symCount = 0;
        for (const auto& sym : sg->symbols)
            if (!symbolsOnly || zbKeepSym.contains(sym.get()))
                symCount++;
        s << symCount;
        for (const auto& sym : sg->symbols)
        {
            if (symbolsOnly && !zbKeepSym.contains(sym.get()))
                continue;
            const auto pit = primRef.constFind(sym->primitive.get());
            if (pit == primRef.cend())
                return QByteArray();
            const auto text = std::dynamic_pointer_cast<const MapPrimitiviser::TextSymbol>(sym);
            const auto icon = std::dynamic_pointer_cast<const MapPrimitiviser::IconSymbol>(sym);
            if (!text && !icon)
                return QByteArray();
            s << static_cast<qint32>(text ? 1 : 2) << *pit;
            s << static_cast<qint32>(sym->location31.x) << static_cast<qint32>(sym->location31.y) << static_cast<qint32>(sym->order)
              << sym->drawAlongPath << sym->intersectsWith << sym->intersectionSizeFactor << sym->intersectionSize
              << sym->intersectionMargin << sym->minDistance << sym->scaleFactor << sym->ignoreClick;
            if (text)
            {
                s << text->baseValue << text->value << static_cast<qint32>(text->languageId) << text->drawOnPath << text->topSide
                  << static_cast<qint32>(text->placement);
                s << static_cast<quint32>(text->additionalPlacements.size());
                for (const auto pl : text->additionalPlacements)
                    s << static_cast<qint32>(pl);
                s << static_cast<qint32>(text->verticalOffset) << static_cast<quint32>(text->color.argb)
                  << static_cast<qint32>(text->size) << static_cast<qint32>(text->shadowRadius)
                  << static_cast<quint32>(text->shadowColor.argb) << static_cast<qint32>(text->wrapWidth)
                  << text->isBold << text->isItalic << text->shieldResourceName << text->underlayIconResourceNames;
            }
            else
            {
                s << icon->resourceName << icon->underlayResourceNames << icon->overlayResourceNames
                  << icon->offsetFactor.x << icon->offsetFactor.y << icon->shieldResourceName;
            }
        }
    }
    return out;
}

bool OsmAnd::MapPrimitiviser_P::zbDeserialize(
    const QByteArray& data,
    const std::shared_ptr<const MapPresentationEnvironment>& env,
    const QHash< QString, std::shared_ptr<const ObfMapSectionInfo> >& sections,
    std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& outPrimitivised,
    QList< std::shared_ptr<const MapObject> >& outTileObjects,
    MapSurfaceType& outSurfaceType)
{
    QDataStream s(data);
    s.setVersion(QDataStream::Qt_5_6);
    s.setFloatingPointPrecision(QDataStream::SinglePrecision);
    quint32 magic;
    qint32 zoom, surface;
    bool coastlineMistake;
    s >> magic;
    if (magic != kZbMagic)
        return false;
    s >> zoom >> surface >> coastlineMistake;
    PointD scaleDivisor;
    scaleDivisor.x = zbReadDouble(s);
    scaleDivisor.y = zbReadDouble(s);

    quint32 objCount, tileObjCount;
    s >> objCount >> tileObjCount;
    QVector< std::shared_ptr<MapObject> > objs(objCount);
    for (quint32 i = 0; i < objCount; i++)
    {
        qint32 kind;
        s >> kind;
        std::shared_ptr<MapObject> o;
        if (kind == 0)
        {
            QString sectionName;
            qint32 minZoom, maxZoom;
            quint64 id, blockId;
            s >> sectionName >> minZoom >> maxZoom >> id >> blockId;
            const auto section = sections.value(sectionName);
            // the attribute mapping is read lazily with the first map block: until a normal read happened, miss
            if (!section || !section->getAttributeMapping())
                return false;
            std::shared_ptr<const ObfMapSectionLevel> level;
            for (const auto& l : section->levels)
            {
                if (static_cast<int>(l->minZoom) == minZoom && static_cast<int>(l->maxZoom) == maxZoom)
                {
                    level = l.shared_ptr();
                    break;
                }
            }
            if (!level)
                return false;
            const auto b = std::make_shared<BinaryMapObject>(section, level);
            b->id.id = id;
            b->blockId.id = blockId;
            o = b;
        }
        else if (kind == 1)
            o = std::make_shared<MapPrimitiviser::SurfaceMapObject>();
        else if (kind == 2)
            o = std::make_shared<MapPrimitiviser::CoastlineMapObject>();
        else
            return false;
        s >> o->attributeIds >> o->additionalAttributeIds >> o->isArea >> o->isCoastline;
        zbReadPoints(s, o->points31);
        quint32 inner;
        s >> inner;
        for (quint32 k = 0; k < inner; k++)
        {
            QVector<PointI> pts;
            zbReadPoints(s, pts);
            o->innerPolygonsPoints31.push_back(qMove(pts));
        }
        qint32 top, left, bottom, right;
        s >> top >> left >> bottom >> right;
        o->bbox31 = AreaI(top, left, bottom, right);
        qint32 labelX, labelY;
        s >> o->captions >> o->captionsOrder >> labelX >> labelY;
        o->labelX = labelX;
        o->labelY = labelY;
        if (!o->isArea && o->points31.size() >= MIN_POINTS_TO_USE_SIMPLIFIED)
        {
            o->vapItems[0] = new VisibleAreaPoints(0, AreaI(), QVector<PointI>());
            o->vapCounts[0].storeRelease(1);
            o->vapItems[1] = new VisibleAreaPoints(0, AreaI(), QVector<PointI>());
            o->vapCounts[1].storeRelease(1);
        }
        objs[i] = o;
    }
    if (s.status() != QDataStream::Ok)
        return false;

    const std::shared_ptr<MapPrimitiviser::PrimitivisedObjects> prim(new MapPrimitiviser::PrimitivisedObjects(
        env, nullptr, static_cast<ZoomLevel>(zoom), scaleDivisor));
    prim->coastlineMistake = coastlineMistake;
    const int defsCount = env->mapStyle->getValueDefinitionsCount();

    QStringList defNames;
    s >> defNames;
    QVector<qint32> defIds(defNames.size());
    for (int i = 0; i < defNames.size(); i++)
    {
        defIds[i] = env->mapStyle->getValueDefinitionIdByName(defNames[i]);
        if (defIds[i] < 0)
            return false;
    }

    QVector< std::shared_ptr<MapPrimitiviser::PrimitivesGroup> > groups;
    quint32 groupCount;
    s >> groupCount;
    groups.reserve(groupCount);
    for (quint32 gi = 0; gi < groupCount; gi++)
    {
        quint32 oi;
        s >> oi;
        if (oi >= objCount)
            return false;
        const std::shared_ptr<MapPrimitiviser::PrimitivesGroup> g(new MapPrimitiviser::PrimitivesGroup(objs[oi]));
        for (int k = 0; k < 3; k++)
        {
            quint32 n;
            s >> n;
            auto& list = k == 0 ? g->polygons : k == 1 ? g->polylines : g->points;
            list.reserve(n);
            for (quint32 i = 0; i < n; i++)
            {
                quint32 type, idx, entries;
                qint32 zOrder;
                qint64 doubledArea;
                float dsf;
                s >> type >> idx >> zOrder >> doubledArea >> dsf >> entries;
                MapStyleEvaluationResult result(defsCount);
                for (quint32 e = 0; e < entries; e++)
                {
                    qint32 defId;
                    QVariant v;
                    s >> defId >> v;
                    if (defId < 0 || defId >= defIds.size())
                        return false;
                    result.setValue(defIds[defId], v);
                }
                const std::shared_ptr<MapPrimitiviser::Primitive> pr(new MapPrimitiviser::Primitive(
                    g, static_cast<MapPrimitiviser::PrimitiveType>(type), idx, result, dsf));
                pr->zOrder = zOrder;
                pr->doubledArea = doubledArea;
                list.push_back(pr);
            }
        }
        groups.push_back(g);
        prim->primitivesGroups.push_back(g);
    }
    const auto resolve =
        [&groups](const quint64 ref) -> std::shared_ptr<const MapPrimitiviser::Primitive>
        {
            const auto gi = static_cast<int>(ref >> 34);
            const auto k = static_cast<int>((ref >> 32) & 3);
            const auto i = static_cast<int>(ref & 0xFFFFFFFFull);
            if (gi >= groups.size())
                return nullptr;
            const auto& list = k == 0 ? groups[gi]->polygons : k == 1 ? groups[gi]->polylines : groups[gi]->points;
            return i < list.size() ? list[i] : nullptr;
        };
    for (int k = 0; k < 3; k++)
    {
        quint32 n;
        s >> n;
        auto& list = k == 0 ? prim->polygons : k == 1 ? prim->polylines : prim->points;
        list.reserve(n);
        for (quint32 i = 0; i < n; i++)
        {
            quint64 ref;
            s >> ref;
            const auto pr = resolve(ref);
            if (!pr)
                return false;
            list.push_back(pr);
        }
    }

    quint32 sgCount;
    s >> sgCount;
    for (quint32 gi = 0; gi < sgCount; gi++)
    {
        quint32 keyIndex, sourceIndex, n;
        bool canBeShownWithoutIcon;
        s >> keyIndex >> sourceIndex >> canBeShownWithoutIcon >> n;
        if (keyIndex >= objCount || sourceIndex >= objCount)
            return false;
        const std::shared_ptr<MapPrimitiviser::SymbolsGroup> sg(new MapPrimitiviser::SymbolsGroup(objs[sourceIndex], canBeShownWithoutIcon));
        for (quint32 i = 0; i < n; i++)
        {
            qint32 kind, x, y, order;
            quint64 ref;
            s >> kind >> ref;
            const auto pr = resolve(ref);
            if (!pr)
                return false;
            std::shared_ptr<MapPrimitiviser::Symbol> sym;
            std::shared_ptr<MapPrimitiviser::TextSymbol> text;
            std::shared_ptr<MapPrimitiviser::IconSymbol> icon;
            if (kind == 1)
                sym = text = std::shared_ptr<MapPrimitiviser::TextSymbol>(new MapPrimitiviser::TextSymbol(pr));
            else
                sym = icon = std::shared_ptr<MapPrimitiviser::IconSymbol>(new MapPrimitiviser::IconSymbol(pr));
            s >> x >> y >> order >> sym->drawAlongPath >> sym->intersectsWith >> sym->intersectionSizeFactor
              >> sym->intersectionSize >> sym->intersectionMargin >> sym->minDistance >> sym->scaleFactor >> sym->ignoreClick;
            sym->location31 = PointI(x, y);
            sym->order = order;
            if (text)
            {
                qint32 languageId, placement, verticalOffset, size, shadowRadius, wrapWidth;
                quint32 color, shadowColor, additional;
                s >> text->baseValue >> text->value >> languageId >> text->drawOnPath >> text->topSide >> placement >> additional;
                for (quint32 a = 0; a < additional; a++)
                {
                    qint32 pl;
                    s >> pl;
                    text->additionalPlacements.push_back(static_cast<MapPrimitiviser::TextSymbol::Placement>(pl));
                }
                s >> verticalOffset >> color >> size >> shadowRadius >> shadowColor >> wrapWidth
                  >> text->isBold >> text->isItalic >> text->shieldResourceName >> text->underlayIconResourceNames;
                text->languageId = static_cast<LanguageId>(languageId);
                text->placement = static_cast<MapPrimitiviser::TextSymbol::Placement>(placement);
                text->verticalOffset = verticalOffset;
                text->color.argb = color;
                text->size = size;
                text->shadowRadius = shadowRadius;
                text->shadowColor.argb = shadowColor;
                text->wrapWidth = wrapWidth;
            }
            else
            {
                s >> icon->resourceName >> icon->underlayResourceNames >> icon->overlayResourceNames
                  >> icon->offsetFactor.x >> icon->offsetFactor.y >> icon->shieldResourceName;
            }
            sg->symbols.push_back(sym);
        }
        prim->symbolsGroups.insert(objs[keyIndex], sg);
    }
    if (s.status() != QDataStream::Ok)
        return false;

    outTileObjects.clear();
    for (quint32 i = 0; i < tileObjCount && i < objCount; i++)
        outTileObjects.push_back(objs[i]);
    outSurfaceType = static_cast<MapSurfaceType>(surface);
    outPrimitivised = prim;
    return true;
}
