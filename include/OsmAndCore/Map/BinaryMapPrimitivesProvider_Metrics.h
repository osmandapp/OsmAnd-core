#ifndef _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_METRICS_H_
#define _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/Map/BinaryMapDataProvider_Metrics.h>
#include <OsmAndCore/Map/Primitiviser_Metrics.h>

namespace OsmAnd
{
    namespace BinaryMapPrimitivesProvider_Metrics
    {
#define OsmAnd__BinaryMapPrimitivesProvider_Metrics__Metric_obtainData__FIELDS(FIELD_ACTION)    \
        /* Total elapsed time */                                                                \
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
                OsmAnd__BinaryMapPrimitivesProvider_Metrics__Metric_obtainData__FIELDS(RESET_METRIC_FIELD);
                obtainBinaryMapDataMetric.reset();
                primitiviseMetric.reset();
            }

            // Metrics from BinaryMapDataProvider
            BinaryMapDataProvider_Metrics::Metric_obtainData obtainBinaryMapDataMetric;

            // Metrics from Primitiviser
            Primitiviser_Metrics::Metric_primitivise primitiviseMetric;

            OsmAnd__BinaryMapPrimitivesProvider_Metrics__Metric_obtainData__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_METRICS_H_)
