#ifndef _OSMAND_CORE_RASTERIZER_METRICS_H_
#define _OSMAND_CORE_RASTERIZER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

namespace OsmAnd
{
    namespace Rasterizer_Metrics
    {
        struct Metric_prepareContext
        {
            inline Metric_prepareContext()
            {
                memset(this, 0, sizeof(Metric_prepareContext));
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_prepareContext));
            }

            // Time spent on objects sorting (seconds)
            float elapsedTimeForSortingObjects;

            // Time spent on polygonizing coastlines (seconds)
            float elapsedTimeForPolygonizingCoastlines;

            // Number of polygonized coastlines
            unsigned int polygonizedCoastlines;

            // Time spent on obtaining primitives (from detailedmap)
            float elapsedTimeForObtainingPrimitivesFromDetailedmap;

            // Time spent on obtaining primitives (from basemap)
            float elapsedTimeForObtainingPrimitivesFromBasemap;

            // Time spent on obtaining primitives (from coastlines)
            float elapsedTimeForObtainingPrimitivesFromCoastlines;

            // Time spent on waiting for future shared primitives groups (for all)
            float elapsedTimeForFutureSharedPrimitivesGroups;

            // Time spent on Order rules evaluation
            float elapsedTimeForOrderEvaluation;

            // Number of order evaluations
            unsigned int orderEvaluations;

            // Number of order rejects
            unsigned int orderRejects;

            // Time spent on Polygon rules evaluation
            float elapsedTimeForPolygonEvaluation;

            // Number of polygon evaluations
            unsigned int polygonEvaluations;

            // Number of polygons that were rejected by their size (area)
            unsigned int polygonsRejectedByArea;

            // Number of obtained polygon primitives
            unsigned int polygonPrimitives;

            // Time spent on Polyline rules evaluation
            float elapsedTimeForPolylineEvaluation;

            // Number of polyline evaluations
            unsigned int polylineEvaluations;

            // Number of obtained polyline primitives
            unsigned int polylinePrimitives;

            // Time spent on Point rules evaluation
            float elapsedTimeForPointEvaluation;

            // Number of point evaluations
            unsigned int pointEvaluations;

            // Number of obtained point primitives
            unsigned int pointPrimitives;

            // Time spent on sorting and filtering primitives
            float elapsedTimeForSortingAndFilteringPrimitives;

            // Time spent on obtaining primitives symbols
            float elapsedTimeForObtainingPrimitivesSymbols;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_RASTERIZER_METRICS_H_)
