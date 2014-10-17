#ifndef _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_METRICS_H_
#define _OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/Map/BinaryMapPrimitivesProvider_Metrics.h>
#include <OsmAndCore/Map/MapRasterizer_Metrics.h>

namespace OsmAnd
{
    namespace BinaryMapRasterLayerProvider_Metrics
    {
#define OsmAnd__BinaryMapRasterLayerProvider_Metrics__Metric_obtainData__FIELDS(FIELD_ACTION)           \
        /* Total elapsed time */                                                                        \
        FIELD_ACTION(float, elapsedTime, "s");
        struct OSMAND_CORE_API Metric_obtainData
        {
            inline Metric_obtainData()
            {
                reset();
            }

            virtual ~Metric_obtainData()
            {
            }

            inline void reset()
            {
                OsmAnd__BinaryMapRasterLayerProvider_Metrics__Metric_obtainData__FIELDS(RESET_METRIC_FIELD);
                obtainBinaryMapPrimitivesMetric.reset();
            }

            // Metrics from BinaryMapPrimitivesProvider
            BinaryMapPrimitivesProvider_Metrics::Metric_obtainData obtainBinaryMapPrimitivesMetric;

            // Metrics from MapRasterizer
            MapRasterizer_Metrics::Metric_rasterize rasterizeMetric;

            OsmAnd__BinaryMapRasterLayerProvider_Metrics__Metric_obtainData__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_RASTER_LAYER_PROVIDER_METRICS_H_)
