#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_READER_METRICS_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_READER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>

namespace OsmAnd
{
    namespace ObfRoutingSectionReader_Metrics
    {
#define OsmAnd__ObfRoutingSectionReader_Metrics__Metric_loadRoads__FIELDS(FIELD_ACTION)             \
        /* Number of visited tree nodes */                                                          \
        FIELD_ACTION(unsigned int, visitedNodes, "");                                               \
                                                                                                    \
        /* Number of accepted tree nodes */                                                         \
        FIELD_ACTION(unsigned int, acceptedNodes, "");                                              \
                                                                                                    \
        /* Elapsed time to check nodes bboxes (in seconds) */                                       \
        FIELD_ACTION(float, elapsedTimeForNodesBbox, "s");                                          \
                                                                                                    \
        /* Elapsed time for tree nodes (in seconds) */                                              \
        FIELD_ACTION(float, elapsedTimeForNodes, "s");                                              \
                                                                                                    \
        /* Number of RoadsBlock processed (read + referenced) */                                    \
        FIELD_ACTION(unsigned int, roadsBlocksProcessed, "");                                       \
                                                                                                    \
        /* Number of RoadsBlock read */                                                             \
        FIELD_ACTION(unsigned int, roadsBlocksRead, "");                                            \
                                                                                                    \
        /* Number of RoadsBlock referenced */                                                       \
        FIELD_ACTION(unsigned int, roadsBlocksReferenced, "");                                      \
                                                                                                    \
        /* Number of visited Roads */                                                               \
        FIELD_ACTION(unsigned int, visitedRoads, "");                                               \
                                                                                                    \
        /* Number of accepted Roads (before filtering) */                                           \
        FIELD_ACTION(unsigned int, acceptedRoads, "");                                              \
                                                                                                    \
        /* Elapsed time for Roads blocks (in seconds) */                                            \
        FIELD_ACTION(float, elapsedTimeForRoadsBlocks, "s");                                        \
                                                                                                    \
        /* Elapsed time for only-visited Roads (in seconds) */                                      \
        FIELD_ACTION(float, elapsedTimeForOnlyVisitedRoads, "s");                                   \
                                                                                                    \
        /* Elapsed time for only-accepted MapObjects (in seconds) */                                \
        FIELD_ACTION(float, elapsedTimeForOnlyAcceptedRoads, "s");                                  \
                                                                                                    \
        /* Elapsed time for processing Roads BBoxes (in seconds) */                                 \
        FIELD_ACTION(float, elapsedTimeForRoadsBbox, "s");                                          \
                                                                                                    \
        /* Elapsed time for processing skipped Road points (in seconds) */                          \
        FIELD_ACTION(float, elapsedTimeForSkippedRoadsPoints, "s");                                 \
                                                                                                    \
        /* Elapsed time for processing not-skipped Road points (in seconds) */                      \
        FIELD_ACTION(float, elapsedTimeForNotSkippedRoadsPoints, "s");

        struct OSMAND_CORE_API Metric_loadRoads : public Metric
        {
            Metric_loadRoads();
            virtual ~Metric_loadRoads();
            virtual void reset();

            OsmAnd__ObfRoutingSectionReader_Metrics__Metric_loadRoads__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_READER_METRICS_H_)
