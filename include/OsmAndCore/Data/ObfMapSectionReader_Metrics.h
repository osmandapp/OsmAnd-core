#ifndef _OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    namespace ObfMapSectionReader_Metrics
    {
        struct OSMAND_CORE_API Metric_loadMapObjects
        {
            inline Metric_loadMapObjects()
            {
                reset();
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_loadMapObjects));
            }

            // Number of visited levels
            unsigned int visitedLevels;

            // Number of accepted levels
            unsigned int acceptedLevels;

            // Elapsed time to check levels bboxes (in seconds)
            float elapsedTimeForLevelsBbox;

            // Number of visited tree nodes
            unsigned int visitedNodes;

            // Number of accepted tree nodes
            unsigned int acceptedNodes;

            // Elapsed time to check nodes bboxes (in seconds)
            float elapsedTimeForNodesBbox;

            // Elapsed time for tree nodes (in seconds)
            float elapsedTimeForNodes;

            // Number of MapObjectBlock processed (read + referenced)
            unsigned int mapObjectsBlocksProcessed;

            // Number of MapObjectBlock read
            unsigned int mapObjectsBlocksRead;

            // Number of MapObjectBlock referenced
            unsigned int mapObjectsBlocksReferenced;

            // Number of visited MapObjects
            unsigned int visitedMapObjects;

            // Number of accepted MapObjects (before filtering)
            unsigned int acceptedMapObjects;

            // Elapsed time for MapObjects (in seconds)
            float elapsedTimeForMapObjectsBlocks;

            // Elapsed time for only-visited MapObjects (in seconds)
            float elapsedTimeForOnlyVisitedMapObjects;

            // Elapsed time for only-accepted MapObjects (in seconds)
            float elapsedTimeForOnlyAcceptedMapObjects;

            // Elapsed time for processing MapObjects BBoxes (in seconds)
            float elapsedTimeForMapObjectsBbox;

            // Elapsed time for processing skipped MapObject points (in seconds)
            float elapsedTimeForSkippedMapObjectsPoints;

            // Number of points read from MapObjects that were skipped
            unsigned int skippedMapObjectsPoints;

            // Elapsed time for processing not-skipped MapObject points (in seconds)
            float elapsedTimeForNotSkippedMapObjectsPoints;

            // Number of points read from MapObjects that were not skipped
            unsigned int notSkippedMapObjectsPoints;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_)
