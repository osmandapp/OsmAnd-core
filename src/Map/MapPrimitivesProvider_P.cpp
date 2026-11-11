#include "MapPrimitivesProvider_P.h"
#include "MapPrimitivesProvider.h"

#include "IMapObjectsProvider.h"
#include "Stopwatch.h"
#include "Utilities.h"
#include "Logging.h"
#include "ObfMapObject.h"
#include "MapRendererPerformanceMetrics.h"

#include "MapPrimitiviser_P.h"
#include "ObfMapObjectsProvider.h"
#include "ObfDataInterface.h"
#include "ObfReader.h"
#include "ObfInfo.h"
#include "ObfFile.h"
#include "ObfMapSectionInfo.h"
#include "IObfsCollection.h"
#include "MapStyleValueDefinition.h"
#include "IMapStyle.h"
#include "MapPresentationEnvironment.h"
#include <QFile>
#include <QReadWriteLock>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <QDir>
#include <chrono>
#include <cstring>
#include <QMutex>
#include <string>
#if defined(__ANDROID__)
#include <sys/system_properties.h>
#endif

namespace
{
    bool zbFlagK()
    {
        static const bool enabled = []() -> bool
        {
            char buffer[96] = { 0 };
#if defined(__ANDROID__)
            __system_property_get("debug.osmand.zb", buffer);
#endif
            return std::string(buffer).find('K') != std::string::npos;
        }();
        return enabled;
    }

    inline void zbMixK(quint64& h, const quint64 v)
    {
        h ^= v + 0x9E3779B97F4A7C15ull + (h << 6) + (h >> 2);
    }

    struct ZbKState
    {
        QMutex mutex;
        bool ready = false;
        quint64 mapsHash = 0;
        QHash< QString, std::shared_ptr<const OsmAnd::ObfMapSectionInfo> > sections;
        QString dir;
        QAtomicInt hits, misses, writes;
        // single pack file
        int fd = -1;
        QMutex appendMutex;
        quint64 end = 0;
        QReadWriteLock indexLock;
        QHash< quint64, QPair<quint64, quint32> > index; // key -> (data offset, data length)
        QAtomicInteger<qint64> rawBytes, packedBytes;
    };

    ZbKState& zbKState();
    const quint32 ZB_PACK_MAGIC = 0x4B50425A; // 'ZBPK'
    const int ZB_PACK_HEADER = 24;
    inline quint32 zbPackCheck(const quint32 rawLen, const quint64 key, const quint32 dataLen)
    {
        return rawLen ^ dataLen ^ static_cast<quint32>(key) ^ static_cast<quint32>(key >> 32) ^ 0xA5C3F00Du;
    }

    void zbPackOpen(ZbKState& st)
    {
        const auto path = st.dir + QLatin1String("zbprim.pack");
        st.fd = ::open(path.toUtf8().constData(), O_RDWR | O_CREAT, 0644);
        if (st.fd < 0)
            return;
        struct stat sb;
        if (fstat(st.fd, &sb) != 0)
            return;
        const auto t0 = std::chrono::steady_clock::now();
        const quint64 size = static_cast<quint64>(sb.st_size);
        quint64 off = 0;
        while (off + ZB_PACK_HEADER <= size)
        {
            quint32 h[6];
            if (pread(st.fd, h, ZB_PACK_HEADER, static_cast<off_t>(off)) != ZB_PACK_HEADER)
                break;
            quint64 key;
            memcpy(&key, &h[2], 8);
            const quint32 rawLen = h[1], dataLen = h[4];
            if (h[0] != ZB_PACK_MAGIC || h[5] != zbPackCheck(rawLen, key, dataLen)
                || off + ZB_PACK_HEADER + dataLen > size)
                break;
            st.index.insert(key, qMakePair(off + ZB_PACK_HEADER, dataLen));
            st.rawBytes.fetchAndAddOrdered(rawLen);
            st.packedBytes.fetchAndAddOrdered(dataLen);
            off += ZB_PACK_HEADER + dataLen;
        }
        if (off != size)
            (void)ftruncate(st.fd, static_cast<off_t>(off));
        st.end = off;
        const auto ms = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, ">>>> 0 PRIMPACK %f: open %d records %lld bytes (raw %lld)",
            ms, st.index.size(), static_cast<long long>(off), static_cast<long long>(st.rawBytes.loadAcquire()));
    }

    QByteArray zbPackRead(const quint64 key)
    {
        auto& st = zbKState();
        QPair<quint64, quint32> loc;
        {
            QReadLocker locker(&st.indexLock);
            const auto it = st.index.constFind(key);
            if (it == st.index.cend())
                return QByteArray();
            loc = *it;
        }
        QByteArray packed(static_cast<int>(loc.second), Qt::Uninitialized);
        if (pread(st.fd, packed.data(), loc.second, static_cast<off_t>(loc.first)) != static_cast<ssize_t>(loc.second))
            return QByteArray();
        return qUncompress(packed);
    }

    void zbPackWrite(const quint64 key, const QByteArray& raw)
    {
        auto& st = zbKState();
        if (st.fd < 0)
            return;
        const auto packed = qCompress(raw, 1);
        quint32 h[6];
        h[0] = ZB_PACK_MAGIC;
        h[1] = static_cast<quint32>(raw.size());
        memcpy(&h[2], &key, 8);
        h[4] = static_cast<quint32>(packed.size());
        h[5] = zbPackCheck(h[1], key, h[4]);
        QMutexLocker locker(&st.appendMutex);
        const auto off = st.end;
        if (pwrite(st.fd, h, ZB_PACK_HEADER, static_cast<off_t>(off)) != ZB_PACK_HEADER
            || pwrite(st.fd, packed.constData(), packed.size(), static_cast<off_t>(off + ZB_PACK_HEADER)) != packed.size())
        {
            (void)ftruncate(st.fd, static_cast<off_t>(off));
            return;
        }
        st.end = off + ZB_PACK_HEADER + packed.size();
        {
            QWriteLocker indexLocker(&st.indexLock);
            st.index.insert(key, qMakePair(off + ZB_PACK_HEADER, static_cast<quint32>(packed.size())));
        }
        st.rawBytes.fetchAndAddOrdered(raw.size());
        st.packedBytes.fetchAndAddOrdered(packed.size());
        st.writes.fetchAndAddOrdered(1);
    }
    ZbKState& zbKState()
    {
        static ZbKState state;
        return state;
    }

    void zbKPrepare(const std::shared_ptr<OsmAnd::IMapObjectsProvider>& mapObjectsProvider)
    {
        auto& st = zbKState();
        QMutexLocker locker(&st.mutex);
        if (st.ready)
            return;
        st.ready = true;
        st.dir = QLatin1String("/sdcard/Android/data/net.osmand.dev/cache/zbprim/");
        QDir().mkpath(st.dir);
        zbPackOpen(st);
        const auto obfProvider = std::dynamic_pointer_cast<const OsmAnd::ObfMapObjectsProvider>(mapObjectsProvider);
        if (!obfProvider)
            return;
        QStringList files;
        for (const auto& f : obfProvider->obfsCollection->getObfFiles())
            files.push_back(f->filePath + QLatin1Char(':') + QString::number(f->fileSize));
        files.sort();
        // FNV-1a: qHash(QString) is seeded per process, the key must survive restarts
        const auto joined = files.join(QLatin1Char('|')).toUtf8();
        quint64 fnv = 1469598103934665603ull;
        for (const char c : joined)
        {
            fnv ^= static_cast<quint8>(c);
            fnv *= 1099511628211ull;
        }
        st.mapsHash = fnv;
        zbMixK(st.mapsHash, static_cast<quint64>(files.size()));
        const auto dataInterface = obfProvider->obfsCollection->obtainDataInterface();
        for (const auto& reader : dataInterface->obfReaders)
        {
            const auto info = reader->obtainInfo();
            if (!info)
                continue;
            for (const auto& section : info->mapSections)
                st.sections.insert(section->name, section.shared_ptr());
        }
    }

    quint64 zbKKey(
        const OsmAnd::MapPrimitivesProvider* owner,
        const OsmAnd::MapPrimitivesProvider::Request& request)
    {
        auto& st = zbKState();
        quint64 h = st.mapsHash;
        const auto& env = owner->primitiviser->environment;
        const auto settings = env->getSettings();
        // order independent: QHash iteration order changes between processes
        quint64 settingsSum = 0;
        for (auto it = settings.cbegin(); it != settings.cend(); ++it)
        {
            // value definition ids and string ids are assigned per process: use the name and the textual value
            const auto& def = env->mapStyle->getValueDefinitionRefById(it.key());
            QString text = def ? def->name : QString::number(it.key());
            text += QLatin1Char('=');
            if (it.value().isComplex)
                text += QLatin1String("complex");
            else if (def && def->dataType == OsmAnd::MapStyleValueDataType::String)
                text += env->mapStyle->getStringById(it.value().asSimple.asUInt);
            else
                text += QString::number(it.value().asSimple.asUInt);
            quint64 e = 1469598103934665603ull;
            for (const char c : text.toUtf8())
            {
                e ^= static_cast<quint8>(c);
                e *= 1099511628211ull;
            }
            settingsSum += e;
        }
        zbMixK(h, settingsSum);
        static QAtomicInt logged(0);
        if (logged.testAndSetOrdered(0, 1))
        {
            QStringList parts;
            for (auto it = settings.cbegin(); it != settings.cend(); ++it)
                parts << QString::number(it.key()) + QLatin1Char('=') + (it.value().isComplex ? QStringLiteral("C") : QString::number(it.value().asSimple.asUInt));
            parts.sort();
            OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Info, ">>>> 0 PRIMKEY 0.0: maps=%llx settingsSum=%llx n=%d dens=%f [%s]",
                st.mapsHash, settingsSum, settings.size(), env->displayDensityFactor, qPrintable(parts.join(QLatin1Char(','))));
        }
        quint32 bits;
        memcpy(&bits, &env->displayDensityFactor, 4); zbMixK(h, bits);
        memcpy(&bits, &env->mapScaleFactor, 4); zbMixK(h, bits);
        memcpy(&bits, &env->symbolsScaleFactor, 4); zbMixK(h, bits);
        zbMixK(h, static_cast<quint64>(owner->mode));
        zbMixK(h, static_cast<quint64>(owner->tileSize));
        zbMixK(h, static_cast<quint64>(request.zoom));
        zbMixK(h, static_cast<quint64>(request.detailedZoom));
        zbMixK(h, static_cast<quint64>(static_cast<quint32>(request.tileId.x)) << 32 | static_cast<quint32>(request.tileId.y));
        return h;
    }
}

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
    // zoom-bench K: restore the whole tile (objects + primitives + symbols) from the disk cache
    // only zoom <= 15 is cached
    bool zbUse = zbFlagK() && request.zoom <= ZoomLevel15;
    quint64 zbKey = 0;
    if (zbUse)
    {
        zbKPrepare(owner->mapObjectsProvider);
        zbKey = zbKKey(owner, request);
        const auto t0 = std::chrono::steady_clock::now();
        const auto bytes = zbPackRead(zbKey);
        if (!bytes.isEmpty())
        {
            std::shared_ptr<const MapPrimitiviser::PrimitivisedObjects> cachedPrimitives;
            QList< std::shared_ptr<const MapObject> > cachedObjects;
            MapSurfaceType cachedSurface = MapSurfaceType::Undefined;
            if (MapPrimitiviser_P::zbDeserialize(bytes, owner->primitiviser->environment, zbKState().sections,
                cachedPrimitives, cachedObjects, cachedSurface))
            {
                const std::shared_ptr<IMapObjectsProvider::Data> cachedTile(new IMapObjectsProvider::Data(
                    request.tileId, request.zoom, cachedSurface, cachedObjects));
                const std::shared_ptr<MapPrimitivesProvider::Data> cachedData(new MapPrimitivesProvider::Data(
                    request.tileId,
                    request.zoom,
                    request.detailedZoom != InvalidZoomLevel ? request.detailedZoom : request.zoom,
                    cachedTile,
                    cachedPrimitives,
                    new RetainableCacheMetadata(tileEntry, nullptr)));
                outTiledPrimitives = cachedData;
                tileEntry->dataIsPresent = true;
                tileEntry->dataWeakRef = cachedData;
                tileEntry->setState(TileState::Loaded);
                {
                    QWriteLocker scopedLocker(&tileEntry->loadedConditionLock);
                    tileEntry->loadedCondition.wakeAll();
                }
                const auto hits = zbKState().hits.fetchAndAddOrdered(1) + 1;
                const auto ms = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                LogPrintf(LogSeverityLevel::Info, ">>>> %lld PRIMCACHE %f: hit %d objects %d bytes from %dx%d@%d (hits %d misses %d)",
                    static_cast<long long>(now), ms, cachedObjects.size(), bytes.size(),
                    request.tileId.x, request.tileId.y, static_cast<int>(request.zoom), hits, zbKState().misses.loadAcquire());
                return true;
            }
        }
        zbKState().misses.fetchAndAddOrdered(1);
    }

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

    // Create tile
    if (zbUse && primitivisedObjects)
    {
        const auto t0 = std::chrono::steady_clock::now();
        const auto bytes = MapPrimitiviser_P::zbSerialize(primitivisedObjects, dataTile->mapObjects, dataTile->tileSurfaceType);
        if (!bytes.isEmpty())
        {
            zbPackWrite(zbKey, bytes);
            const auto ms = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            LogPrintf(LogSeverityLevel::Info, ">>>> 0 PRIMPACKW %f: %d raw bytes @%d (pack raw %lld packed %lld)",
                ms, bytes.size(), static_cast<int>(request.zoom),
                static_cast<long long>(zbKState().rawBytes.loadAcquire()),
                static_cast<long long>(zbKState().packedBytes.loadAcquire()));
        }
    }

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
