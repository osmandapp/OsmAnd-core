#ifndef _OSMAND_CORE_MAP_RENDERER_PERFORMANCE_METRICS_H_
#define _OSMAND_CORE_MAP_RENDERER_PERFORMANCE_METRICS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QMutex>

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

        Stopwatch syncTimer;
        Stopwatch syncUnloadCollectTimer;
        Stopwatch syncUnloadGPUTimer;
        Stopwatch syncUploadCollectTimer;
        Stopwatch syncUploadGPUTimer;
        Stopwatch intersectionTimer;

        QMutex mutex;
        QHash<quint64, Stopwatch*> readTimers;
        QHash<quint64, Stopwatch*> primitivesTimers;
        QHash<quint64, Stopwatch*> rasterTimers;
        QHash<quint64, Stopwatch*> textTimers;

        MapRendererPerformanceMetrics();
        virtual ~MapRendererPerformanceMetrics();

        bool enabled;
                
        void startSymbolsLoading(const ZoomLevel zoom);
        void stopSymbolsLoading(const ZoomLevel zoom);
        
        // --- Read Stage ---
        int totalRead;
        float totalReadDuration;
        float lastReadTime;
        void readStart(const TileId tileId);
        void readFinish(const TileId tileId, const ZoomLevel zoom, const int allMapObjectsCount, const int sharedMapObjectsCount, const float allocationTime, ObfMapObjectsProvider_Metrics::Metric_obtainData* const metric = nullptr);

        // --- Primitives Stage ---
        int totalPrimitives;
        float totalPrimitivesDuration;
        float lastPrimitivesTime;
        void primitivesStart(const TileId tileId);
        void primitivesFinish(const TileId tileId, const ZoomLevel zoom, const int polygons, const int polylines, const int points, MapPrimitivesProvider_Metrics::Metric_obtainData* const metric = nullptr);

        // --- Raster Stage ---
        int totalRaster;
        float totalRasterDuration;
        float lastRasterTime;
        void rasterStart(const TileId tileId);
        void rasterFinish(const TileId tileId, const ZoomLevel zoom, MapRasterLayerProvider_Metrics::Metric_obtainData* const metric = nullptr);

        // --- Text Stage ---
        int totalText;
        float totalTextDuration;
        float lastTextTime;
        void textStart(const TileId tileId);
        void textFinish(const TileId tileId, const ZoomLevel zoom, const int spriteSymbols, const int onPathSymbols);
        
        // --- Sync Stage ---
        int totalSync;
        float totalSyncDuration;
        float lastSyncTime;
        void syncStart();
        void syncFinish(const int resourcesUploadedCount, const int resourcesUnloadedCount);
        int totalUnloadedIcons;
        int totalUnloadedCaptions;
        void unloadedFromGPU(const int unloadedIcons, const int unloadedCaptions);
        int totalUploadedIcons;
        int totalUploadedCaptions;
        void uploadedToGPU(const int uploadedIcons, const int uploadedCaptions);

        int totalSyncUnloadCollect;
        float totalSyncUnloadCollectDuration;
        void syncUnloadCollectStart();
        void syncUnloadCollectFinish(const int count);
        int totalSyncUnloadGPU;
        float totalSyncUnloadGPUDuration;
        void syncUnloadGPUStart();
        void syncUnloadGPUFinish(const int count);

        int totalSyncUploadCollect;
        float totalSyncUploadCollectDuration;
        void syncUploadCollectStart();
        void syncUploadCollectFinish(const int count);
        int totalSyncUploadGPU;
        float totalSyncUploadGPUDuration;
        void syncUploadGPUStart();
        void syncUploadGPUFinish(const int count);

        // --- Intersection Stage ---
        void intersectionStart();
        void intersectionFinish(const int renderableSymbols);
    };
}

#endif // !defined(_OSMAND_CORE_MAP_RENDERER_PERFORMANCE_METRICS_H_)
