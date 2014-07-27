#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_METRICS_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/BinaryMapPrimitivesProvider_Metrics.h>
#include <OsmAndCore/Map/MapRasterizer_Metrics.h>

namespace OsmAnd
{
    namespace BinaryMapRasterBitmapTileProvider_Metrics
    {
        struct Metric_obtainData
        {
            inline Metric_obtainData()
            {
                reset();
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_obtainData));
                obtainBinaryMapPrimitivesMetric.reset();
            }

            // Metrics from BinaryMapPrimitivesProvider
            BinaryMapPrimitivesProvider_Metrics::Metric_obtainData obtainBinaryMapPrimitivesMetric;

            // Metrics from MapRasterizer
            MapRasterizer_Metrics::Metric_rasterize rasterizeMetric;

            // Total elapsed time
            float elapsedTime;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_BITMAP_TILE_PROVIDER_METRICS_H_)
