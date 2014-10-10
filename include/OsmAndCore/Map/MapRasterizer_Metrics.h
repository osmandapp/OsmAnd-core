#ifndef _OSMAND_CORE_MAP_RASTERIZER_METRICS_H_
#define _OSMAND_CORE_MAP_RASTERIZER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>

namespace OsmAnd
{
    namespace MapRasterizer_Metrics
    {
#define OsmAnd__MapRasterizer_Metrics__Metric_rasterize__FIELDS(FIELD_ACTION)       \
        /* Total elapsed time */                                                    \
        FIELD_ACTION(float, elapsedTime, "s");
        struct OSMAND_CORE_API Metric_rasterize
        {
            inline Metric_rasterize()
            {
                reset();
            }

            virtual ~Metric_rasterize()
            {
            }

            inline void reset()
            {
                OsmAnd__MapRasterizer_Metrics__Metric_rasterize__FIELDS(RESET_METRIC_FIELD);
            }

            OsmAnd__MapRasterizer_Metrics__Metric_rasterize__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_MAP_RASTERIZER_METRICS_H_)
