#ifndef _OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>

namespace OsmAnd
{
    namespace ObfMapSectionReader_Metrics
    {
#define OsmAnd__ObfMapSectionReader_Metrics__Metric_loadMapObjects__FIELDS(FIELD_ACTION)        \
        /* Number of visited levels */                                                          \
        FIELD_ACTION(unsigned int, visitedLevels, "");                                          \
                                                                                                \
        /* Number of accepted levels */                                                         \
        FIELD_ACTION(unsigned int, acceptedLevels, "");                                         \
                                                                                                \
        /* Elapsed time to check levels bboxes (in seconds) */                                  \
        FIELD_ACTION(float, elapsedTimeForLevelsBbox, "s");                                     \
                                                                                                \
        /* Number of visited tree nodes */                                                      \
        FIELD_ACTION(unsigned int, visitedNodes, "");                                           \
                                                                                                \
        /* Number of accepted tree nodes */                                                     \
        FIELD_ACTION(unsigned int, acceptedNodes, "");                                          \
                                                                                                \
        /* Elapsed time to check nodes bboxes (in seconds) */                                   \
        FIELD_ACTION(float, elapsedTimeForNodesBbox, "s");                                      \
                                                                                                \
        /* Elapsed time for tree nodes (in seconds) */                                          \
        FIELD_ACTION(float, elapsedTimeForNodes, "s");                                          \
                                                                                                \
        /* Number of MapObjectBlock processed (read + referenced) */                            \
        FIELD_ACTION(unsigned int, mapObjectsBlocksProcessed, "");                              \
                                                                                                \
        /* Number of MapObjectBlock read */                                                     \
        FIELD_ACTION(unsigned int, mapObjectsBlocksRead, "");                                   \
                                                                                                \
        /* Number of MapObjectBlock referenced */                                               \
        FIELD_ACTION(unsigned int, mapObjectsBlocksReferenced, "");                             \
                                                                                                \
        /* Number of visited MapObjects */                                                      \
        FIELD_ACTION(unsigned int, visitedMapObjects, "");                                      \
                                                                                                \
        /* Number of accepted MapObjects (before filtering) */                                  \
        FIELD_ACTION(unsigned int, acceptedMapObjects, "");                                     \
                                                                                                \
        /* Elapsed time for MapObjects (in seconds) */                                          \
        FIELD_ACTION(float, elapsedTimeForMapObjectsBlocks, "s");                               \
                                                                                                \
        /* Elapsed time for only-visited MapObjects (in seconds) */                             \
        FIELD_ACTION(float, elapsedTimeForOnlyVisitedMapObjects, "s");                          \
                                                                                                \
        /* Elapsed time for only-accepted MapObjects (in seconds) */                            \
        FIELD_ACTION(float, elapsedTimeForOnlyAcceptedMapObjects, "s");                         \
                                                                                                \
        /* Elapsed time for processing MapObjects BBoxes (in seconds) */                        \
        FIELD_ACTION(float, elapsedTimeForMapObjectsBbox, "s");                                 \
                                                                                                \
        /* Elapsed time for processing skipped MapObject points (in seconds) */                 \
        FIELD_ACTION(float, elapsedTimeForSkippedMapObjectsPoints, "s");                        \
                                                                                                \
        /* Number of points read from MapObjects that were skipped */                           \
        FIELD_ACTION(unsigned int, skippedMapObjectsPoints, "");                                \
                                                                                                \
        /* Elapsed time for processing not-skipped MapObject points (in seconds) */             \
        FIELD_ACTION(float, elapsedTimeForNotSkippedMapObjectsPoints, "s");                     \
                                                                                                \
        /* Number of points read from MapObjects that were not skipped */                       \
        FIELD_ACTION(unsigned int, notSkippedMapObjectsPoints, "");

        struct OSMAND_CORE_API Metric_loadMapObjects : public Metric
        {
            Metric_loadMapObjects();
            virtual ~Metric_loadMapObjects();
            virtual void reset();

            OsmAnd__ObfMapSectionReader_Metrics__Metric_loadMapObjects__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_)
