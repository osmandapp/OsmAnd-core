#ifndef _OSMAND_CORE_MAP_RENDERER_PERFORMANCE_METRICS_H_
#define _OSMAND_CORE_MAP_RENDERER_PERFORMANCE_METRICS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Stopwatch.h>
#include <OsmAndCore/Map/MapPrimitivesProvider_Metrics.h>
#include <OsmAndCore/Map/MapRasterLayerProvider_Metrics.h>
#include <OsmAndCore/Map/ObfMapObjectsProvider_Metrics.h>

namespace OsmAnd
{
    class Stopwatch;

    struct OSMAND_CORE_API MapRendererPerformanceMetrics
    {
        Stopwatch symbolsLoadingTimer;
        Stopwatch readTimer;
        Stopwatch primitivesTimer;
        Stopwatch rasterTimer;
        Stopwatch textTimer;
        Stopwatch syncTimer;
        Stopwatch intersectionTimer;

        MapRendererPerformanceMetrics();
        virtual ~MapRendererPerformanceMetrics();

        bool enabled;
                
        void startSymbolsLoading(const ZoomLevel zoom);
        void stopSymbolsLoading(const ZoomLevel zoom);
        
        int totalRead;
        float totalReadDuration;
        float lastReadTime;
        void readStart();
        void readFinish(const TileId tileId, const ZoomLevel zoom, const int allMapObjectsCount, const int sharedMapObjectsCount, ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric = nullptr);

        int totalPrimitives;
        float totalPrimitivesDuration;
        float lastPrimitivesTime;
        void primitivesStart();
        void primitivesFinish(const TileId tileId, const ZoomLevel zoom, const int polygons, const int polylines, const int points, MapPrimitivesProvider_Metrics::Metric_obtainData* const metric = nullptr);

        int totalRaster;
        float totalRasterDuration;
        float lastRasterTime;
        void rasterStart();
        void rasterFinish(const TileId tileId, const ZoomLevel zoom, MapRasterLayerProvider_Metrics::Metric_obtainData* const metric = nullptr);

        int totalText;
        float totalTextDuration;
        float lastTextTime;
        void textStart();
        void textFinish(const TileId tileId, const ZoomLevel zoom, const int spriteSymbols, const int onPathSymbols);
        
        int totalSync;
        float totalSyncDuration;
        float lastSyncTime;
        void syncStart();
        void syncFinish(const int resourcesUploadedCount, const int resourcesUnloadedCount);
        
        void intersectionStart();
        void intersectionFinish(const int renderableSymbols);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_PERFORMANCE_METRICS_H_)
