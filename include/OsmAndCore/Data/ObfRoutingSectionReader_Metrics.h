#ifndef _OSMAND_CORE_OBF_ROUTING_SECTION_READER_METRICS_H_
#define _OSMAND_CORE_OBF_ROUTING_SECTION_READER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace ObfRoutingSectionReader_Metrics
    {
        struct OSMAND_CORE_API Metric_loadRoads
        {
            inline Metric_loadRoads()
            {
                reset();
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_loadRoads));
            }

            // Number of visited tree nodes
            unsigned int visitedNodes;

            // Number of accepted tree nodes
            unsigned int acceptedNodes;

            // Elapsed time to check nodes bboxes (in seconds)
            float elapsedTimeForNodesBbox;

            // Elapsed time for tree nodes (in seconds)
            float elapsedTimeForNodes;

            // Number of RoadsBlock processed (read + referenced)
            unsigned int roadsBlocksProcessed;

            // Number of RoadsBlock read
            unsigned int roadsBlocksRead;

            // Number of RoadsBlock referenced
            unsigned int roadsBlocksReferenced;

            // Number of visited Roads
            unsigned int visitedRoads;

            // Number of accepted Roads (before filtering)
            unsigned int acceptedRoads;

            // Elapsed time for Roads blocks (in seconds)
            float elapsedTimeForRoadsBlocks;

            // Elapsed time for only-visited Roads (in seconds)
            float elapsedTimeForOnlyVisitedRoads;

            // Elapsed time for only-accepted MapObjects (in seconds)
            float elapsedTimeForOnlyAcceptedRoads;

            // Elapsed time for processing Roads BBoxes (in seconds)
            float elapsedTimeForRoadsBbox;

            // Elapsed time for processing skipped Road points (in seconds)
            float elapsedTimeForSkippedRoadsPoints;

            // Elapsed time for processing not-skipped Road points (in seconds)
            float elapsedTimeForNotSkippedRoadsPoints;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_OBF_ROUTING_SECTION_READER_METRICS_H_)
