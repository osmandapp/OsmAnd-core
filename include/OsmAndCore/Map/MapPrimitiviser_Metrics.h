#ifndef _OSMAND_CORE_MAP_PRIMITIVISER_METRICS_H_
#define _OSMAND_CORE_MAP_PRIMITIVISER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace MapPrimitiviser_Metrics
    {
#define OsmAnd__MapPrimitiviser_Metrics__Metric_primitivise__FIELDS(FIELD_ACTION)                   \
        /* Time spent on obtaining primitives */                                                    \
        FIELD_ACTION(float, elapsedTimeForPrimitives, "s");                                         \
                                                                                                    \
        /* Time spent on obtaining all primitives groups */                                         \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesGroups, "s");                          \
                                                                                                    \
        /* Time spent on waiting for future shared primitives groups (for all) */                   \
        FIELD_ACTION(float, elapsedTimeForFutureSharedPrimitivesGroups, "s");                       \
                                                                                                    \
        /* Time spent on Order rules evaluation */                                                  \
        FIELD_ACTION(float, elapsedTimeForOrderEvaluation, "s");                                    \
                                                                                                    \
        /* Number of order evaluations */                                                           \
        FIELD_ACTION(unsigned int, orderEvaluations, "");                                           \
                                                                                                    \
        /* Number of order rejects */                                                               \
        FIELD_ACTION(unsigned int, orderRejects, "");                                               \
                                                                                                    \
        /* Time spent on Order processing */                                                        \
        FIELD_ACTION(float, elapsedTimeForOrderProcessing, "s");                                    \
                                                                                                    \
        /* Time spent on Polygon rules evaluation */                                                \
        FIELD_ACTION(float, elapsedTimeForPolygonEvaluation, "s");                                  \
                                                                                                    \
        /* Number of polygon evaluations */                                                         \
        FIELD_ACTION(unsigned int, polygonEvaluations, "");                                         \
                                                                                                    \
        /* Number of polygon rejectes */                                                            \
        FIELD_ACTION(unsigned int, polygonRejects, "");                                             \
                                                                                                    \
        /* Number of polygons that were rejected by their size (area) */                            \
        FIELD_ACTION(unsigned int, polygonsRejectedByArea, "");                                     \
                                                                                                    \
        /* Number of obtained polygon primitives */                                                 \
        FIELD_ACTION(unsigned int, polygonPrimitives, "");                                          \
                                                                                                    \
        /* Time spent on Polygon processing */                                                      \
        FIELD_ACTION(float, elapsedTimeForPolygonProcessing, "s");                                  \
                                                                                                    \
        /* Time spent on Polyline rules evaluation */                                               \
        FIELD_ACTION(float, elapsedTimeForPolylineEvaluation, "s");                                 \
                                                                                                    \
        /* Number of polyline evaluations */                                                        \
        FIELD_ACTION(unsigned int, polylineEvaluations, "");                                        \
                                                                                                    \
        /* Number of polylines rejected by style */                                                 \
        FIELD_ACTION(unsigned int, polylineRejects, "");                                            \
                                                                                                    \
        /* Number of polylines rejected by density */                                               \
        FIELD_ACTION(unsigned int, polylineRejectedByDensity, "");                                  \
                                                                                                    \
        /* Number of obtained polyline primitives */                                                \
        FIELD_ACTION(unsigned int, polylinePrimitives, "");                                         \
                                                                                                    \
        /* Time spent on Polyline processing */                                                     \
        FIELD_ACTION(float, elapsedTimeForPolylineProcessing, "s");                                 \
                                                                                                    \
        /* Time spent on Point rules evaluation */                                                  \
        FIELD_ACTION(float, elapsedTimeForPointEvaluation, "s");                                    \
                                                                                                    \
        /* Number of point evaluations */                                                           \
        FIELD_ACTION(unsigned int, pointEvaluations, "");                                           \
                                                                                                    \
        /* Number of point evaluations */                                                           \
        FIELD_ACTION(unsigned int, pointRejects, "");                                               \
                                                                                                    \
        /* Number of obtained point primitives */                                                   \
        FIELD_ACTION(unsigned int, pointPrimitives, "");                                            \
                                                                                                    \
        /* Time spent on Point processing */                                                        \
        FIELD_ACTION(float, elapsedTimeForPointProcessing, "s");                                    \
                                                                                                    \
        /* Time spent on sorting and filtering primitives */                                        \
        FIELD_ACTION(float, elapsedTimeForSortingAndFilteringPrimitives, "s");                      \
                                                                                                    \
        /* Time spent on obtaining primitives symbols */                                            \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesSymbols, "s");                         \
                                                                                                    \
        /* Time spent on processing symbols groups */                                               \
        FIELD_ACTION(float, elapsedTimeForSymbolsGroupsProcessing, "s");                            \
                                                                                                    \
        /* Number of processed symbols groups */                                                    \
        FIELD_ACTION(unsigned int, symbolsGroupsProcessed, "");                                     \
                                                                                                    \
        /* Time spent on text symbols evaluation */                                                 \
        FIELD_ACTION(float, elapsedTimeForTextSymbolsEvaluation, "s");                              \
                                                                                                    \
        /* Number of text symbols evaluations */                                                    \
        FIELD_ACTION(unsigned int, textSymbolsEvaluations, "");                                     \
                                                                                                    \
        /* Time spent on processing text symbols */                                                 \
        FIELD_ACTION(float, elapsedTimeForTextSymbolsProcessing, "s");                              \
                                                                                                    \
        /* Number of rejected text symbols */                                                       \
        FIELD_ACTION(unsigned int, rejectedTextSymbols, "");                                        \
                                                                                                    \
        /* Number of obtained text symbols */                                                       \
        FIELD_ACTION(unsigned int, obtainedTextSymbols, "");                                        \
                                                                                                    \
        /* Time spent on processing icon symbols */                                                 \
        FIELD_ACTION(float, elapsedTimeForIconSymbolsProcessing, "s");                              \
                                                                                                    \
        /* Number of rejected icon symbols */                                                       \
        FIELD_ACTION(unsigned int, rejectedIconSymbols, "");                                        \
                                                                                                    \
        /* Number of obtained icon symbols */                                                       \
        FIELD_ACTION(unsigned int, obtainedIconSymbols, "");                                        \
                                                                                                    \
        /* Time spent totally */                                                                    \
        FIELD_ACTION(float, elapsedTime, "s");

        struct OSMAND_CORE_API Metric_primitivise : public Metric
        {
        protected:
            Metric_primitivise();
        public:
            virtual ~Metric_primitivise();
            virtual void reset();

            OsmAnd__MapPrimitiviser_Metrics__Metric_primitivise__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };


#define OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseAllMapObjects__FIELDS(FIELD_ACTION)      \
        /* Time spent on obtaining primitives */                                                    \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitives, "s");

        struct OSMAND_CORE_API Metric_primitiviseAllMapObjects : public Metric_primitivise
        {
            Metric_primitiviseAllMapObjects();
            virtual ~Metric_primitiviseAllMapObjects();
            virtual void reset();

            OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseAllMapObjects__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };

#define OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithoutSurface__FIELDS(FIELD_ACTION)     \
        /* Time spent on objects sorting (seconds) */                                               \
        FIELD_ACTION(float, elapsedTimeForSortingObjects, "s");                                     \
                                                                                                    \
        /* Time spent on obtaining primitives (from detailedmap) */                                 \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesFromDetailedmap, "s");                 \
                                                                                                    \
        /* Time spent on obtaining primitives (from basemap) */                                     \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesFromBasemap, "s");

        struct OSMAND_CORE_API Metric_primitiviseWithoutSurface : public Metric_primitivise
        {
            Metric_primitiviseWithoutSurface();
            virtual ~Metric_primitiviseWithoutSurface();
            virtual void reset();

            OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithoutSurface__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };

#define OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithSurface__FIELDS(FIELD_ACTION)        \
        /* Time spent on polygonizing coastlines (seconds) */                                       \
        FIELD_ACTION(float, elapsedTimeForPolygonizingCoastlines, "s");                             \
                                                                                                    \
        /* Number of polygonized coastlines */                                                      \
        FIELD_ACTION(unsigned int, polygonizedCoastlines, "");                                      \
                                                                                                    \
        /* Time spent on obtaining primitives (from coastlines) */                                  \
        FIELD_ACTION(float, elapsedTimeForObtainingPrimitivesFromCoastlines, "s");

        struct OSMAND_CORE_API Metric_primitiviseWithSurface : public Metric_primitiviseWithoutSurface
        {
            Metric_primitiviseWithSurface();
            virtual ~Metric_primitiviseWithSurface();
            virtual void reset();

            OsmAnd__MapPrimitiviser_Metrics__Metric_primitiviseWithSurface__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_MAP_PRIMITIVISER_METRICS_H_)
