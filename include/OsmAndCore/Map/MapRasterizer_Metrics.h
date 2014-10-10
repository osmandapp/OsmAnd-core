#ifndef _OSMAND_CORE_MAP_RASTERIZER_METRICS_H_
#define _OSMAND_CORE_MAP_RASTERIZER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    namespace MapRasterizer_Metrics
    {
        struct OSMAND_CORE_API Metric_rasterize
        {
            inline Metric_rasterize()
            {
                reset();
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_rasterize));
            }

            // Total elapsed time
            float elapsedTime;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_MAP_RASTERIZER_METRICS_H_)
