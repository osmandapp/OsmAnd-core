#ifndef _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_METRICS_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/Data/ObfMapSectionReader_Metrics.h>

namespace OsmAnd
{
    namespace BinaryMapDataProvider_Metrics
    {
#define OsmAnd__BinaryMapDataProvider_Metrics__Metric_obtainData__FIELDS(FIELD_ACTION)          \
        /* Elapsed time on obtaining OBF interface */                                           \
        FIELD_ACTION(float, elapsedTimeForObtainingObfInterface, "s");                          \
                                                                                                \
        /* Elapsed time on filtering BinaryMapObjects by their ID to skip loaded ones */        \
        FIELD_ACTION(float, elapsedTimeForObjectsFiltering, "s");                               \
                                                                                                \
        /* Elapsed time on read */                                                              \
        FIELD_ACTION(float, elapsedTimeForRead, "s");                                           \
                                                                                                \
        /* Elapsed time (total) */                                                              \
        FIELD_ACTION(float, elapsedTime, "s");                                                  \
                                                                                                \
        /* Total number of obtained objects */                                                  \
        FIELD_ACTION(unsigned int, objectsCount, "");                                           \
                                                                                                \
        /* Number of obtained unique objects */                                                 \
        FIELD_ACTION(unsigned int, uniqueObjectsCount, "");                                     \
                                                                                                \
        /* Number of obtained shared objects */                                                 \
        FIELD_ACTION(unsigned int, sharedObjectsCount, "");
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
                OsmAnd__BinaryMapDataProvider_Metrics__Metric_obtainData__FIELDS(RESET_METRIC_FIELD);
                loadMapObjectsMetric.reset();
            }

            // Metric from ObfMapSectionReader
            ObfMapSectionReader_Metrics::Metric_loadMapObjects loadMapObjectsMetric;

            OsmAnd__BinaryMapDataProvider_Metrics__Metric_obtainData__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_METRICS_H_)
