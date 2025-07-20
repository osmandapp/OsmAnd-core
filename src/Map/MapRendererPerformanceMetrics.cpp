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

void OsmAnd::MapRendererPerformanceMetrics::startSymbolsLoading()
{
    symbolsLoadingTimer.start();
    
    primitivesTimer.start();
    rasterTimer.start();
    textTimer.start();
    syncTimer.start();
    intersectionTimer.start();

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
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld START 0.0:", millis);
}

void OsmAnd::MapRendererPerformanceMetrics::stopSymbolsLoading()
{
    auto time_since_epoch = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(time_since_epoch).count();
    LogPrintf(LogSeverityLevel::Info, ">>>> %ld FINISH %f: TOTAL read %d %f, primitives %d %f, raster %d %f, text %d %f, sync %d %f", millis, symbolsLoadingTimer.elapsed(),
        totalRead, totalReadDuration,
        totalPrimitives, totalPrimitivesDuration,
        totalRaster, totalRasterDuration,
        totalText, totalTextDuration,
        totalSync, totalSyncDuration);
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
        LogPrintf(LogSeverityLevel::Info, ">>>> %ld SYNC %f: syncResourcesInGPU %ld uploaded, %ld unloaded",
                  millis, syncTimer.elapsed(), resourcesUploadedCount, resourcesUnloadedCount);
    }
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
