#ifndef _OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_
#define _OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd
{
    namespace ObfMapSectionReader_Metrics
    {
        struct Metric_loadMapObjects
        {
            inline Metric_loadMapObjects()
            {
                memset(this, 0, sizeof(Metric_loadMapObjects));
            }

            // Number of visited levels
            unsigned int visitedLevels;

            // Number of accepted levels
            unsigned int acceptedLevels;

            // Number of visited tree nodes
            unsigned int visitedNodes;

            // Number of accepted tree nodes
            unsigned int acceptedNodes;

            // Elapsed time for tree nodes (in seconds)
            float elapsedTimeForNodes;

            // Number of MapObjectBlock read
            unsigned int mapObjectsBlocksRead;

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
        };

    } // namespace ObfMapSectionReader_Metrics

} // namespace OsmAnd

#endif // !defined(_OSMAND_CORE_OBF_MAP_SECTION_READER_METRICS_H_)
