#ifndef _OSMAND_CORE_BINARY_MAP_OBJECTS_PROVIDER_METRICS_H_
#define _OSMAND_CORE_BINARY_MAP_OBJECTS_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>

namespace OsmAnd
{
    namespace ObfMapObjectsProvider_Metrics
    {
#define OsmAnd__ObfMapObjectsProvider_Metrics__Metric_obtainData__FIELDS(FIELD_ACTION)          \
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
        struct OSMAND_CORE_API Metric_obtainData : public Metric
        {
            Metric_obtainData();
            virtual ~Metric_obtainData();
            virtual void reset();

            OsmAnd__ObfMapObjectsProvider_Metrics__Metric_obtainData__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_OBJECTS_PROVIDER_METRICS_H_)
