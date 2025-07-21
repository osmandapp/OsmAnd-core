#include "MapRendererPerformanceMetrics.h"

#include "Logging.h"

OsmAnd::MapRendererPerformanceMetrics::MapRendererPerformanceMetrics()
    : enabled(true)
//    , disableSymbolsStage(false)
{
}

OsmAnd::MapRendererPerformanceMetrics::~MapRendererPerformanceMetrics()
{
}

void OsmAnd::MapRendererPerformanceMetrics::startSymbolsLoading(const ZoomLevel zoom)
{
    symbolsLoadingTimer.start();

    totalRead = 0;
    totalReadDuration = 0.0f;
    lastReadTime = 0.0f;
    
    totalPrimitives = 0;
    totalPrimitivesDuration = 0.0f;
    lastPrimitivesTime = 0.0f;
    
    totalRaster = 0;
    totalRasterDuration = 0.0f;
    lastRasterTime = 0.0f;
    
    totalText = 0;
    totalTextDuration = 0.0f;
    lastTextTime = 0.0f;
    
    totalSync = 0;
    totalSyncDuration = 0.0f;
    lastSyncTime = 0.0f;
    
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld START 0.0: %dz", millis, zoom);
}

void OsmAnd::MapRendererPerformanceMetrics::stopSymbolsLoading(const ZoomLevel zoom)
{
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

void OsmAnd::MapRendererPerformanceMetrics::readStart()
{
    readTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::readFinish(const TileId tileId, const ZoomLevel zoom, const int allMapObjectsCount, const int sharedMapObjectsCount, ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric/* = nullptr*/)
{
    totalRead++;
    totalReadDuration += readTimer.elapsed();
    lastReadTime = symbolsLoadingTimer.elapsed();
    
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld READ %f: %d map objects (%d unique, %d shared) read from %dx%d@%d",
        millis, readTimer.elapsed(),
        allMapObjectsCount,
        allMapObjectsCount - sharedMapObjectsCount,
        sharedMapObjectsCount,
        tileId.x, tileId.y, zoom);
    //qPrintable(metric ? metric->toString(false, QLatin1String("\t - ")) : QLatin1String("(null)")));
}

void OsmAnd::MapRendererPerformanceMetrics::primitivesStart()
{
    primitivesTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::primitivesFinish(const TileId tileId, const ZoomLevel zoom, const int polygons, const int polylines, const int points, MapPrimitivesProvider_Metrics::Metric_obtainData* const metric/* = nullptr*/)
{
    totalPrimitives++;
    totalPrimitivesDuration += primitivesTimer.elapsed();
    lastPrimitivesTime = symbolsLoadingTimer.elapsed();
    
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld PRIMITIVES %f: %d polygons, %d polylines, %d points primitivised from %dx%d@%d", millis, primitivesTimer.elapsed(), polygons, polylines, points, tileId.x, tileId.y, zoom);
    //qPrintable(metric ? metric->toString(false, QLatin1String("\t - ")) : QLatin1String("(null)")));
}

void OsmAnd::MapRendererPerformanceMetrics::rasterStart()
{
    rasterTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::rasterFinish(const TileId tileId, const ZoomLevel zoom, MapRasterLayerProvider_Metrics::Metric_obtainData* const metric/* = nullptr*/)
{
    totalRaster++;
    totalRasterDuration += rasterTimer.elapsed();
    lastRasterTime = symbolsLoadingTimer.elapsed();
    
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld RASTER %f: %dx%d@%d rasterized on CPU",
              millis, rasterTimer.elapsed(), tileId.x, tileId.y, zoom);
    //qPrintable(metric ? metric->toString(QLatin1String("\t - ")) : QLatin1String("(null)")));
}

void OsmAnd::MapRendererPerformanceMetrics::textStart()
{
    textTimer.start();
}

void OsmAnd::MapRendererPerformanceMetrics::textFinish(const TileId tileId, const ZoomLevel zoom, const int spriteSymbols, const int onPathSymbols)
{
    totalText++;
    totalTextDuration += textTimer.elapsed();
    lastTextTime = symbolsLoadingTimer.elapsed();
    
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld TEXT %f: %d sprite, %d onPath obtained from %dx%d@%d",
        millis, textTimer.elapsed(), spriteSymbols, onPathSymbols, tileId.x, tileId.y, zoom);
}

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
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld SYNC %f: upload %f (%d) %f (%d) unload %f (%d) %f (%d)",
                  millis, syncTimer.elapsed(),
                  totalSyncUploadCollectDuration, totalSyncUploadCollect,
                  totalSyncUploadGPUDuration, totalSyncUploadGPU,
                  totalSyncUnloadCollectDuration, totalSyncUnloadCollect,
                  totalSyncUnloadGPUDuration, totalSyncUnloadGPU);
    }
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
