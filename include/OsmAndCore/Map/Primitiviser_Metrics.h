#ifndef _OSMAND_CORE_PRIMITIVISER_METRICS_H_
#define _OSMAND_CORE_PRIMITIVISER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace Primitiviser_Metrics
    {
#define OsmAnd__Primitiviser_Metrics__Metric_primitivise__FIELDS(FIELD_ACTION)              \
        /* Time spent on objects sorting (seconds) */                                       \
        FIELD_ACTION(float, elapsedTimeForSortingObjects, "s");                             \
                                                                                            \
        /* Time spent on polygonizing coastlines (seconds) */                               \
        FIELD_ACTION(float, elapsedTimeForPolygonizingCoastlines, "s");                     \
                                                                                            \
        /* Number of polygonized coastlines */                                              \
        FIELD_ACTION(unsigned int, polygonizedCoastlines, "");                              \
                                                                                            \
        /* Time spent on obtaining primitives (from detailedmap) */                         \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesFromDetailedmap, "s");         \
                                                                                            \
        /* Time spent on obtaining primitives (from basemap) */                             \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesFromBasemap, "s");             \
                                                                                            \
        /* Time spent on obtaining primitives (from coastlines) */                          \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesFromCoastlines, "s");          \
                                                                                            \
        /* Time spent on obtaining all primitives groups */                                 \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesGroups, "s");                  \
                                                                                            \
        /* Time spent on waiting for future shared primitives groups (for all) */           \
        FIELD_ACTION(float, elapsedTimeForFutureSharedPrimitivesGroups, "s");               \
                                                                                            \
        /* Time spent on Order rules evaluation */                                          \
        FIELD_ACTION(float, elapsedTimeForOrderEvaluation, "s");                            \
                                                                                            \
        /* Number of order evaluations */                                                   \
        FIELD_ACTION(unsigned int, orderEvaluations, "");                                   \
                                                                                            \
        /* Number of order rejects */                                                       \
        FIELD_ACTION(unsigned int, orderRejects, "");                                       \
                                                                                            \
        /* Time spent on Polygon rules evaluation */                                        \
        FIELD_ACTION(float, elapsedTimeForPolygonEvaluation, "s");                          \
                                                                                            \
        /* Number of polygon evaluations */                                                 \
        FIELD_ACTION(unsigned int, polygonEvaluations, "");                                 \
                                                                                            \
        /* Number of polygon rejectes */                                                    \
        FIELD_ACTION(unsigned int, polygonRejects, "");                                     \
                                                                                            \
        /* Number of polygons that were rejected by their size (area) */                    \
        FIELD_ACTION(unsigned int, polygonsRejectedByArea, "");                             \
                                                                                            \
        /* Number of obtained polygon primitives */                                         \
        FIELD_ACTION(unsigned int, polygonPrimitives, "");                                  \
                                                                                            \
        /* Time spent on Polyline rules evaluation */                                       \
        FIELD_ACTION(float, elapsedTimeForPolylineEvaluation, "s");                         \
                                                                                            \
        /* Number of polyline evaluations */                                                \
        FIELD_ACTION(unsigned int, polylineEvaluations, "");                                \
                                                                                            \
        /* Number of polyline evaluations */                                                \
        FIELD_ACTION(unsigned int, polylineRejects, "");                                    \
                                                                                            \
        /* Number of obtained polyline primitives */                                        \
        FIELD_ACTION(unsigned int, polylinePrimitives, "");                                 \
                                                                                            \
        /* Time spent on Point rules evaluation */                                          \
        FIELD_ACTION(float, elapsedTimeForPointEvaluation, "s");                            \
                                                                                            \
        /* Number of point evaluations */                                                   \
        FIELD_ACTION(unsigned int, pointEvaluations, "");                                   \
                                                                                            \
        /* Number of point evaluations */                                                   \
        FIELD_ACTION(unsigned int, pointRejects, "");                                       \
                                                                                            \
        /* Number of obtained point primitives */                                           \
        FIELD_ACTION(unsigned int, pointPrimitives, "");                                    \
                                                                                            \
        /* Time spent on sorting and filtering primitives */                                \
        FIELD_ACTION(float, elapsedTimeForSortingAndFilteringPrimitives, "s");              \
                                                                                            \
        /* Time spent on obtaining primitives symbols */                                    \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesSymbols, "s");                 \
                                                                                            \
        /* Time spent totally */                                                            \
        FIELD_ACTION(float, elapsedTime, "s");

        struct OSMAND_CORE_API Metric_primitivise : public Metric
        {
            Metric_primitivise();
            virtual ~Metric_primitivise();
            virtual void reset();

            OsmAnd__Primitiviser_Metrics__Metric_primitivise__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_PRIMITIVISER_METRICS_H_)
