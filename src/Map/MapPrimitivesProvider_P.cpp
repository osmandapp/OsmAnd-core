#include "MapPrimitivesProvider_P.h"
#include "MapPrimitivesProvider.h"

#include "IMapObjectsProvider.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"
#include "ObfMapObject.h"
#include "MapRendererPerformanceMetrics.h"
#include "MapPrimitiviser_P.h"
#include "MapPresentationEnvironment.h"
#include "MapStyleValueDefinition.h"
#include "IMapStyle.h"
#include "IMapTiledSymbolsProvider.h"
#include "ObfMapObjectsProvider.h"
#include "IObfsCollection.h"
#include "ObfDataInterface.h"
#include "ObfReader.h"
#include "ObfFile.h"
#include "ObfInfo.h"
#include "ObfMapSectionInfo.h"

namespace
{
    // FNV-1a: qHash() is seeded per process, the disk cache keys must survive restarts
    struct DiskCacheHash
    {
        uint64_t value = 1469598103934665603ull;

        void add(const QByteArray& bytes)
        {
            for (const char c : bytes)
            {
                value ^= static_cast<uint8_t>(c);
                value *= 1099511628211ull;
            }
            add(static_cast<uint64_t>(bytes.size()));
        }

        void add(const uint64_t v)
        {
            for (int i = 0; i < 8; i++)
            {
                value ^= static_cast<uint8_t>(v >> (i * 8));
                value *= 1099511628211ull;
            }
        }

        void add(const float v)
        {
            uint32_t bits;
            memcpy(&bits, &v, sizeof(bits));
            add(static_cast<uint64_t>(bits));
        }
    };

    // the maps are looked at again at most this often
    const qint64 kDiskCacheMapsCheckMs = 3000;
}

OsmAnd::MapPrimitivesProvider_P::MapPrimitivesProvider_P(MapPrimitivesProvider* owner_)
    : _diskCacheMapsSignature(0)
    , _diskCacheMapsSynced(false)
    , _primitiviserCache(new MapPrimitiviser::Cache())
    , owner(owner_)
{
}

void OsmAnd::MapPrimitivesProvider_P::setDiskCache(const std::shared_ptr<MapTileDiskCache>& diskCache, const QString& styleKey)
{
    QMutexLocker scopedLocker(&_diskCacheMutex);
    _diskCache = diskCache;
    _diskCacheStyleKey = styleKey;
    _diskCacheMapsSynced = false;
    _diskCacheMapsSignature = 0;
}

std::shared_ptr<OsmAnd::MapTileDiskCache> OsmAnd::MapPrimitivesProvider_P::getDiskCache(
    const ZoomLevel zoom,
    uint64_t& outEnvironment) const
{
    if (!MapTileDiskCache::isZoomCached(zoom) || owner->mode != MapPrimitivesProvider::Mode::WithSurface)
        return nullptr;
    std::shared_ptr<MapTileDiskCache> diskCache;
    {
        QMutexLocker scopedLocker(&_diskCacheMutex);
        diskCache = _diskCache;
    }
    if (!diskCache || !diskCache->isOpen() || !syncDiskCacheMaps(diskCache))
        return nullptr;
    outEnvironment = computeDiskCacheEnvironment();
    diskCache->useEnvironment(outEnvironment);
    return diskCache;
}

uint64_t OsmAnd::MapPrimitivesProvider_P::computeDiskCacheEnvironment() const
{
    const auto& env = owner->primitiviser->environment;
    DiskCacheHash hash;
    hash.add(static_cast<uint64_t>(MapPrimitiviser_P::DiskCacheFormatVersion));
    {
        QMutexLocker scopedLocker(&_diskCacheMutex);
        hash.add(_diskCacheStyleKey.toUtf8());
    }

    // ids of value definitions and strings are given out per process: use names and textual values
    QStringList settings;
    const auto values = env->getSettings();
    for (auto it = values.cbegin(); it != values.cend(); ++it)
    {
        const auto& definition = env->mapStyle->getValueDefinitionRefById(it.key());
        QString text = definition ? definition->name : QString::number(it.key());
        text += QLatin1Char('=');
        if (it.value().isComplex)
            text += QStringLiteral("complex");
        else if (definition && definition->dataType == MapStyleValueDataType::String)
            text += env->mapStyle->getStringById(it.value().asSimple.asUInt);
        else
            text += QString::number(it.value().asSimple.asUInt);
        settings.push_back(text);
    }
    settings.sort();
    hash.add(settings.join(QLatin1Char('\n')).toUtf8());

    QStringList disabled = env->disabledAttributes.values();
    disabled.sort();
    hash.add(disabled.join(QLatin1Char('\n')).toUtf8());
    hash.add(env->getLocaleLanguageId().toUtf8());
    hash.add(static_cast<uint64_t>(env->getLanguagePreference()));
    hash.add(env->displayDensityFactor);
    hash.add(env->mapScaleFactor);
    hash.add(env->symbolsScaleFactor);
    hash.add(static_cast<uint64_t>(owner->tileSize));
    hash.add(static_cast<uint64_t>(owner->mode));

    if (_diskCacheLastEnvironment.fetchAndStoreOrdered(hash.value) != hash.value)
    {
        LogPrintf(LogSeverityLevel::Info, "Map tile disk cache environment %016llx: lang=%s/%d density=%.3f/%.3f/%.3f %s",
            static_cast<unsigned long long>(hash.value), qPrintable(env->getLocaleLanguageId()),
            static_cast<int>(env->getLanguagePreference()), env->displayDensityFactor, env->mapScaleFactor,
            env->symbolsScaleFactor, qPrintable(settings.join(QLatin1Char(' '))));
    }
    return hash.value;
}

bool OsmAnd::MapPrimitivesProvider_P::syncDiskCacheMaps(const std::shared_ptr<MapTileDiskCache>& diskCache) const
{
    QMutexLocker scopedLocker(&_diskCacheMutex);
    if (_diskCacheMapsSynced && _diskCacheMapsCheck.isValid() && _diskCacheMapsCheck.elapsed() < kDiskCacheMapsCheckMs)
        return true;
    _diskCacheMapsCheck.start();

    const auto obfProvider = std::dynamic_pointer_cast<const ObfMapObjectsProvider>(owner->mapObjectsProvider);
    if (!obfProvider)
        return false;
    QStringList files;
    for (const auto& file : obfProvider->obfsCollection->getObfFiles())
        files.push_back(file->filePath + QLatin1Char(':') + QString::number(file->fileSize));
    files.sort();
    DiskCacheHash signature;
    signature.add(files.join(QLatin1Char('\n')).toUtf8());
    if (_diskCacheMapsSynced && signature.value == _diskCacheMapsSignature)
        return true;

    // a map that is being installed or updated right now is left out: its area is dropped now and when it is back
    const auto dataInterface = obfProvider->obfsCollection->obtainDataInterface(
        nullptr, MinZoomLevel, MaxZoomLevel, ObfDataTypesMask().set(ObfDataType::Map), false);
    QList<MapTileDiskCache::MapFile> maps;
    QHash< QString, std::shared_ptr<const ObfMapSectionInfo> > sections;
    for (const auto& reader : dataInterface->obfReaders)
    {
        const auto info = reader->obtainInfo();
        if (!info)
            continue;
        bool hasArea = false;
        AreaI bbox31;
        for (const auto& section : info->mapSections)
        {
            sections.insert(MapPrimitiviser_P::getDiskCacheSectionKey(*section), section.shared_ptr());
            for (const auto& level : section->levels)
            {
                if (!hasArea)
                    bbox31 = level->area31;
                bbox31.top() = qMin(bbox31.top(), level->area31.top());
                bbox31.left() = qMin(bbox31.left(), level->area31.left());
                bbox31.bottom() = qMax(bbox31.bottom(), level->area31.bottom());
                bbox31.right() = qMax(bbox31.right(), level->area31.right());
                hasArea = true;
            }
        }
        const auto& path = reader->obfFile->filePath;
        // OsmAnd Live updates come every hour or day and would wipe whole regions: their changes reach the
        // cached zooms with the next map update or when the tile is evicted
        if (!hasArea || path.contains(QStringLiteral("/live/")))
            continue;
        MapTileDiskCache::MapFile map;
        map.path = path;
        map.size = static_cast<int64_t>(reader->obfFile->fileSize);
        map.timestamp = static_cast<int64_t>(info->creationTimestamp);
        map.bbox31 = bbox31;
        maps.push_back(map);
    }
    const auto deleted = diskCache->syncMaps(maps);
    if (deleted > 0)
    {
        LogPrintf(LogSeverityLevel::Info, "Map tile disk cache: %d entries dropped after map changes (%d maps)",
            deleted, maps.size());
    }
    _diskCacheSections = sections;
    _diskCacheMapsSignature = signature.value;
    _diskCacheMapsSynced = true;
    return true;
}

bool OsmAnd::MapPrimitivesProvider_P::obtainCachedSymbols(
    const MapPrimitivesProvider::Request& request,
    std::shared_ptr<MapPrimitivesProvider::Data>& outTiledPrimitives)
{
    uint64_t environment;
    const auto diskCache = getDiskCache(request.zoom, environment);
    if (!diskCache)
        return false;
    MapTileDiskCache::Key key;
    key.environment = environment;
    key.kind = MapTileDiskCache::DataKind::Symbols;
    key.tileId = request.tileId;
    key.zoom = request.zoom;
    key.variant = request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom;
    const auto data = diskCache->get(key);
    if (data.isEmpty())
        return false;

    QHash< QString, std::shared_ptr<const ObfMapSectionInfo> > sections;
    {
        QMutexLocker scopedLocker(&_diskCacheMutex);
        sections = _diskCacheSections;
    }
    std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects> primitivisedObjects;
    QList< std::shared_ptr<const MapObject> > mapObjects;
    MapSurfaceType surfaceType = MapSurfaceType::Undefined;
    if (!MapPrimitiviser_P::deserializeSymbols(data, owner->primitiviser->environment, sections,
        primitivisedObjects, mapObjects, surfaceType))
    {
        return false;
    }
    // not published in the tile collection: these primitives hold symbols only and may not reach a raster tile
    const std::shared_ptr<IMapObjectsProvider::Data> mapObjectsData(new IMapObjectsProvider::Data(
        request.tileId, request.zoom, surfaceType, mapObjects));
    outTiledPrimitives.reset(new MapPrimitivesProvider::Data(
        request.tileId,
        request.zoom,
        static_cast<ZoomLevel>(key.variant),
        mapObjectsData,
        primitivisedObjects));
    return true;
}

void OsmAnd::MapPrimitivesProvider_P::storeCachedSymbols(
    const MapPrimitivesProvider::Request& request,
    const std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects>& primitivisedObjects,
    const QList< std::shared_ptr<const MapObject> >& mapObjects,
    const MapSurfaceType surfaceType)
{
    // primitives with the detail of another zoom serve a zoom animation only, see MapRasterLayerProvider_P
    if (request.detailedZoom != InvalidZoomLevel && request.detailedZoom != request.zoom)
        return;
    uint64_t environment;
    const auto diskCache = getDiskCache(request.zoom, environment);
    if (!diskCache || !primitivisedObjects)
        return;
    MapTileDiskCache::Key key;
    key.environment = environment;
    key.kind = MapTileDiskCache::DataKind::Symbols;
    key.tileId = request.tileId;
    key.zoom = request.zoom;
    key.variant = request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom;
    if (diskCache->contains(key))
        return;
    const auto data = MapPrimitiviser_P::serializeSymbols(primitivisedObjects, surfaceType);
    if (!data.isEmpty())
        diskCache->put(key, data);
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

    const Stopwatch totalStopwatch(metric != nullptr);

    // a symbols request is served from the disk cache when it can, the raster tile comes from its own entry
    const bool isSymbolsRequest = dynamic_cast<const IMapTiledSymbolsProvider::Request*>(&request) != nullptr;
    if (isSymbolsRequest && obtainCachedSymbols(request, outTiledPrimitives))
    {
        if (metric)
            metric->elapsedTime = totalStopwatch.elapsed();
        return true;
    }

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

            if (outTiledPrimitives)
            {
                if (request.detailedZoom == InvalidZoomLevel || request.detailedZoom == outTiledPrimitives->detailedZoom)
                    return true;
                else
                    outTiledPrimitives.reset();
            }

            // Otherwise consider this tile entry as expired, remove it from collection (it's safe to do that right now)
            // This will enable creation of new entry on next loop cycle
            _tileReferences.removeEntry(request.tileId, request.zoom);
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
        _tileReferences.removeEntry(request.tileId, request.zoom);
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
        _tileReferences.removeEntry(request.tileId, request.zoom);
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
            request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom,
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
        _tileReferences.removeEntry(request.tileId, request.zoom);
        tileEntry.reset();

        return false;
    }

    storeCachedSymbols(request, primitivisedObjects, dataTile->mapObjects, dataTile->tileSurfaceType);

    // Create tile
    const std::shared_ptr<MapPrimitivesProvider::Data> newTiledData(new MapPrimitivesProvider::Data(
        request.tileId,
        request.zoom,
        request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom,
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
