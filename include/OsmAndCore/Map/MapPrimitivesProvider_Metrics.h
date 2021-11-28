#ifndef _OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_METRICS_H_
#define _OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>

namespace OsmAnd
{
    namespace MapPrimitivesProvider_Metrics
    {
#define OsmAnd__MapPrimitivesProvider_Metrics__Metric_obtainData__FIELDS(FIELD_ACTION)          \
        /* Total elapsed time */                                                                \
        FIELD_ACTION(float, elapsedTime, "s");
        struct OSMAND_CORE_API Metric_obtainData : public Metric
        {
            Metric_obtainData();
            virtual ~Metric_obtainData();
            virtual void reset();

            OsmAnd__MapPrimitivesProvider_Metrics__Metric_obtainData__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVES_PROVIDER_METRICS_H_)
