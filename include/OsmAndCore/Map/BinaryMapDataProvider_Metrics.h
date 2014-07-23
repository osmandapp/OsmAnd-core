#ifndef _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_METRICS_H_
#define _OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/ObfMapSectionReader_Metrics.h>
#include <OsmAndCore/Map/Rasterizer_Metrics.h>

namespace OsmAnd
{
    namespace BinaryMapDataProvider_Metrics
    {
        struct Metric_obtainData
        {
            inline Metric_obtainData()
            {
                reset();
            }

            inline void reset()
            {
                loadMapObjectsMetric.reset();
                prepareContextMetric.reset();
                elapsedTimeForObtainingObfInterface = 0.0f;
                elapsedTimeForObjectsFiltering = 0.0f;
                elapsedTimeForRead = 0.0f;
                elapsedTime = 0.0f;
                objectsCount = 0;
                uniqueObjectsCount = 0;
                sharedObjectsCount = 0;
            }

            // Metric from ObfMapSectionReader
            ObfMapSectionReader_Metrics::Metric_loadMapObjects loadMapObjectsMetric;

            // Metric from Rasterizer
            Rasterizer_Metrics::Metric_prepareContext prepareContextMetric;

            // Elapsed time on obtaining OBF interface
            float elapsedTimeForObtainingObfInterface;

            // Elapsed time on filtering BinaryMapObjects by their ID to skip loaded ones
            float elapsedTimeForObjectsFiltering;

            // Elapsed time on read
            float elapsedTimeForRead;

            // Elapsed time (total)
            float elapsedTime;

            // Total number of obtained objects
            unsigned int objectsCount;

            // Number of obtained unique objects
            unsigned int uniqueObjectsCount;

            // Number of obtained shared objects
            unsigned int sharedObjectsCount;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_BINARY_MAP_DATA_PROVIDER_METRICS_H_)
