#ifndef _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_METRICS_H_
#define _OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>
#include <OsmAndCore/Map/BinaryMapDataProvider_Metrics.h>
#include <OsmAndCore/Map/Primitiviser_Metrics.h>

namespace OsmAnd
{
    namespace BinaryMapPrimitivesProvider_Metrics
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
                obtainBinaryMapDataMetric.reset();
                primitiviseMetric.reset();
            }

            // Metrics from BinaryMapDataProvider
            BinaryMapDataProvider_Metrics::Metric_obtainData obtainBinaryMapDataMetric;

            // Metrics from Primitiviser
            Primitiviser_Metrics::Metric_primitivise primitiviseMetric;

            // Time spent totally
            float elapsedTime;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_PRIMITIVES_PROVIDER_METRICS_H_)
