#include "MapRendererPerformanceMetrics.h"

#include "Logging.h"
#include <QMutexLocker>

OsmAnd::MapRendererPerformanceMetrics::MapRendererPerformanceMetrics()
    : enabled(false)
{
}

OsmAnd::MapRendererPerformanceMetrics::~MapRendererPerformanceMetrics()
{
    qDeleteAll(readTimers);
    qDeleteAll(primitivesTimers);
    qDeleteAll(rasterTimers);
    qDeleteAll(textTimers);
}

void OsmAnd::MapRendererPerformanceMetrics::startSymbolsLoading(const ZoomLevel zoom)
{
    QMutexLocker locker(&mutex);

    symbolsLoadingTimer.start();

    totalRead = 0;
    totalReadDuration = 0.0f;
    lastReadTime = 0.0f;
    qDeleteAll(readTimers);
    readTimers.clear();
    
    totalPrimitives = 0;
    totalPrimitivesDuration = 0.0f;
    lastPrimitivesTime = 0.0f;
    qDeleteAll(primitivesTimers);
    primitivesTimers.clear();
    
    totalRaster = 0;
    totalRasterDuration = 0.0f;
    lastRasterTime = 0.0f;
    qDeleteAll(rasterTimers);
    rasterTimers.clear();
    
    totalText = 0;
    totalTextDuration = 0.0f;
    lastTextTime = 0.0f;
    qDeleteAll(textTimers);
    textTimers.clear();
    
    totalSync = 0;
    totalSyncDuration = 0.0f;
    lastSyncTime = 0.0f;
    
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld START 0.0: %dz", millis, zoom);
}

void OsmAnd::MapRendererPerformanceMetrics::stopSymbolsLoading(const ZoomLevel zoom)
{
    QMutexLocker locker(&mutex);
    
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld FINISH %f: %dz Read %f (%d) Primitives %f (%d) Raster %f (%d) Text %f (%d) Sync %f (%d)",
        millis, symbolsLoadingTimer.elapsed(), zoom,
        totalReadDuration, totalRead,
        totalPrimitivesDuration, totalPrimitives,
        totalRasterDuration, totalRaster,
        totalTextDuration, totalText,
        totalSyncDuration, totalSync);
}

// --- Read Stage ---

void OsmAnd::MapRendererPerformanceMetrics::readStart(const TileId tileId)
{
    QMutexLocker locker(&mutex);

    readTimers.insert(tileId.id, new Stopwatch(true));
}

void OsmAnd::MapRendererPerformanceMetrics::readFinish(const TileId tileId, const ZoomLevel zoom, const int allMapObjectsCount, const int sharedMapObjectsCount, const float allocationTime, ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric/* = nullptr*/)
{
    Stopwatch* timer = nullptr;
    float elapsed = 0.0f;
    
    {
        QMutexLocker locker(&mutex);
        
        timer = readTimers.take(tileId.id);
        if (timer)
        {
            elapsed = timer->elapsed();
            totalRead++;
            totalReadDuration += elapsed + allocationTime;
            lastReadTime = symbolsLoadingTimer.elapsed();
        }
    }
    
    if (timer)
    {
        delete timer;
        
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld READ %f: %d map objects (%f alloc) read from %dx%d@%d",
            millis, elapsed,
            allMapObjectsCount, allocationTime,
            tileId.x, tileId.y, zoom);
        //qPrintable(metric ? metric->toString(false, QLatin1String("\t - ")) : QLatin1String("(null)")));
    }
}

// --- Primitives Stage ---

void OsmAnd::MapRendererPerformanceMetrics::primitivesStart(const TileId tileId)
{
    QMutexLocker locker(&mutex);
    
    primitivesTimers.insert(tileId.id, new Stopwatch(true));
}

void OsmAnd::MapRendererPerformanceMetrics::primitivesFinish(const TileId tileId, const ZoomLevel zoom, const int polygons, const int polylines, const int points, MapPrimitivesProvider_Metrics::Metric_obtainData* const metric/* = nullptr*/)
{
    Stopwatch* timer = nullptr;
    float elapsed = 0.0f;

    {
        QMutexLocker locker(&mutex);
        
        timer = primitivesTimers.take(tileId.id);
        if (timer)
        {
            elapsed = timer->elapsed();
            totalPrimitives++;
            totalPrimitivesDuration += elapsed;
            lastPrimitivesTime = symbolsLoadingTimer.elapsed();
        }
    }
    
    if (timer)
    {
        delete timer;
        
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld PRIMITIVES %f: %d polygons, %d polylines, %d points primitivised from %dx%d@%d", millis, elapsed, polygons, polylines, points, tileId.x, tileId.y, zoom);
        //qPrintable(metric ? metric->toString(false, QLatin1String("\t - ")) : QLatin1String("(null)")));
    }
}

// --- Raster Stage ---

void OsmAnd::MapRendererPerformanceMetrics::rasterStart(const TileId tileId)
{
    QMutexLocker locker(&mutex);
    
    rasterTimers.insert(tileId.id, new Stopwatch(true));
}

void OsmAnd::MapRendererPerformanceMetrics::rasterFinish(const TileId tileId, const ZoomLevel zoom, MapRasterLayerProvider_Metrics::Metric_obtainData* const metric/* = nullptr*/)
{
    Stopwatch* timer = nullptr;
    float elapsed = 0.0f;

    {
        QMutexLocker locker(&mutex);
        
        timer = rasterTimers.take(tileId.id);
        if (timer)
        {
            elapsed = timer->elapsed();
            totalRaster++;
            totalRasterDuration += elapsed;
            lastRasterTime = symbolsLoadingTimer.elapsed();
        }
    }
    
    if (timer)
    {
        delete timer;
        
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld RASTER %f: %dx%d@%d rasterized on CPU",
                  millis, elapsed, tileId.x, tileId.y, zoom);
        //qPrintable(metric ? metric->toString(QLatin1String("\t - ")) : QLatin1String("(null)")));
    }
}

// --- Text Stage ---

void OsmAnd::MapRendererPerformanceMetrics::textStart(const TileId tileId)
{
    QMutexLocker locker(&mutex);
    
    textTimers.insert(tileId.id, new Stopwatch(true));
}

void OsmAnd::MapRendererPerformanceMetrics::textFinish(const TileId tileId, const ZoomLevel zoom, const int spriteSymbols, const int onPathSymbols)
{
    Stopwatch* timer = nullptr;
    float elapsed = 0.0f;
    
    {
        QMutexLocker locker(&mutex);
        
        timer = textTimers.take(tileId.id);
        if (timer)
        {
            elapsed = timer->elapsed();
            totalText++;
            totalTextDuration += elapsed;
            lastTextTime = symbolsLoadingTimer.elapsed();
        }
    }
    
    if (timer)
    {
        delete timer;
        
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld TEXT %f: %d sprite, %d onPath obtained from %dx%d@%d",
            millis, elapsed, spriteSymbols, onPathSymbols, tileId.x, tileId.y, zoom);
    }
}

// --- Sync Stage ---

void OsmAnd::MapRendererPerformanceMetrics::syncStart()
{
    syncTimer.start();
    
    totalSyncUnloadCollect = 0;
    totalSyncUnloadCollectDuration = 0.0;
    totalSyncUnloadGPU = 0;
    totalSyncUnloadGPUDuration = 0.0;

    totalSyncUploadCollect = 0;
    totalSyncUploadCollectDuration = 0.0;
    totalSyncUploadGPU = 0;
    totalSyncUploadGPUDuration = 0.0;
    
    totalUnloadedIcons = 0;
    totalUnloadedCaptions = 0;
    totalUploadedIcons = 0;
    totalUploadedCaptions = 0;
}

void OsmAnd::MapRendererPerformanceMetrics::syncFinish(const int resourcesUploadedCount, const int resourcesUnloadedCount)
{
    totalSync++;
    totalSyncDuration += syncTimer.elapsed();
    lastSyncTime = symbolsLoadingTimer.elapsed();

    if (resourcesUploadedCount > 0 || resourcesUnloadedCount > 0)
    {
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld SYNC %f: upload %f (%d) %f (%d-%d) unload %f (%d) %f (%d-%d)",
                  millis, syncTimer.elapsed(),
                  totalSyncUploadCollectDuration, totalSyncUploadCollect,
                  totalSyncUploadGPUDuration, totalUploadedIcons, totalUploadedCaptions,
                  totalSyncUnloadCollectDuration, totalSyncUnloadCollect,
                  totalSyncUnloadGPUDuration, totalUnloadedIcons, totalUnloadedCaptions);
    }
}

void OsmAnd::MapRendererPerformanceMetrics::unloadedFromGPU(const int unloadedIcons, const int unloadedCaptions)
{
    totalUnloadedIcons += unloadedIcons;
    totalUnloadedCaptions += unloadedCaptions;
}

void OsmAnd::MapRendererPerformanceMetrics::uploadedToGPU(const int uploadedIcons, const int uploadedCaptions)
{
    totalUploadedIcons += uploadedIcons;
    totalUploadedCaptions += uploadedCaptions;
}

void OsmAnd::MapRendererPerformanceMetrics::syncUnloadCollectStart()
{
    syncUnloadCollectTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::syncUnloadCollectFinish(const int count)
{
    totalSyncUnloadCollect += count;
    totalSyncUnloadCollectDuration += syncUnloadCollectTimer.elapsed();
}

void OsmAnd::MapRendererPerformanceMetrics::syncUnloadGPUStart()
{
    syncUnloadGPUTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::syncUnloadGPUFinish(const int count)
{
    totalSyncUnloadGPU += count;
    totalSyncUnloadGPUDuration += syncUnloadGPUTimer.elapsed();
}

void OsmAnd::MapRendererPerformanceMetrics::syncUploadCollectStart()
{
    syncUploadCollectTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::syncUploadCollectFinish(const int count)
{
    totalSyncUploadCollect += count;
    totalSyncUploadCollectDuration += syncUploadCollectTimer.elapsed();
}

void OsmAnd::MapRendererPerformanceMetrics::syncUploadGPUStart()
{
    syncUploadGPUTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::syncUploadGPUFinish(const int count)
{
    totalSyncUploadGPU += count;
    totalSyncUploadGPUDuration += syncUploadGPUTimer.elapsed();
}

// --- Intersection Stage ---

void OsmAnd::MapRendererPerformanceMetrics::intersectionStart()
{
    intersectionTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::intersectionFinish(const int renderableSymbols)
{
    if (renderableSymbols > 0)
    {
        auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
        auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld INTERSECTION %f: plottedSymbols=%d",
                  millis, intersectionTimer.elapsed(), renderableSymbols);
    }
}
