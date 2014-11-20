#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_METRICS_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/Map/IMapRenderer_Metrics.h>

namespace OsmAnd
{
    namespace AtlasMapRenderer_Metrics
    {
#define OsmAnd__AtlasMapRenderer_Metrics__Metric_renderFrame__FIELDS(FIELD_ACTION)                              \
        /* Time elapsed for sky stage */                                                                        \
        FIELD_ACTION(float, elapsedTimeForSkyStage, "s");                                                       \
                                                                                                                \
        /* Time elapsed for map layers stage */                                                                 \
        FIELD_ACTION(float, elapsedTimeForMapLayersStage, "s");                                                 \
                                                                                                                \
        /* Time elapsed for symbols stage */                                                                    \
        FIELD_ACTION(float, elapsedTimeForSymbolsStage, "s");                                                   \
        FIELD_ACTION(float, elapsedTimeForPreparingSymbols, "s");                                               \
        FIELD_ACTION(float, elapsedTimeForObtainingRenderableSymbols, "s");                                     \
        FIELD_ACTION(float, elapsedTimeForObtainingRenderableSymbolsNoLock, "s");                               \
        FIELD_ACTION(float, elapsedTimeForObtainingRenderableSymbolsOnlyLock, "s");                             \
        FIELD_ACTION(float, elapsedTimeForObtainRenderableSymbolCalls, "s");                                    \
        FIELD_ACTION(unsigned int, obtainRenderableSymbolCalls, "");                                            \
        FIELD_ACTION(float, elapsedTimeForPlotSymbolCalls, "s");                                                \
        FIELD_ACTION(unsigned int, plotSymbolCalls, "");                                                        \
        FIELD_ACTION(unsigned int, plotSymbolCallsSucceeded, "");                                               \
        FIELD_ACTION(unsigned int, plotSymbolCallsFailed, "");                                                  \
        FIELD_ACTION(float, elapsedTimeForApplyIntersectionWithOtherSymbolsFilteringCalls, "s");                \
        FIELD_ACTION(unsigned int, applyIntersectionWithOtherSymbolsFilteringCalls, "");                        \
        FIELD_ACTION(unsigned int, acceptedIntersectionWithOtherSymbolsFiltering, "");                          \
        FIELD_ACTION(unsigned int, rejectedIntersectionWithOtherSymbolsFiltering, "");                          \
        FIELD_ACTION(float, elapsedTimeForApplyMinDistanceToSameContentFromOtherSymbolFilteringCalls, "s");     \
        FIELD_ACTION(unsigned int, applyMinDistanceToSameContentFromOtherSymbolFilteringCalls, "");             \
        FIELD_ACTION(unsigned int, acceptedMinDistanceToSameContentFromOtherSymbolFiltering, "");               \
        FIELD_ACTION(unsigned int, rejectedMinDistanceToSameContentFromOtherSymbolFiltering, "");               \
        FIELD_ACTION(float, elapsedTimeForAddToIntersectionsCalls, "s");                                        \
        FIELD_ACTION(unsigned int, addToIntersectionsCalls, "");                                                \
        FIELD_ACTION(unsigned int, acceptedByAddToIntersections, "");                                           \
        FIELD_ACTION(unsigned int, rejectedByAddToIntersections, "");                                           \
        FIELD_ACTION(float, elapsedTimeForSymbolsPresentationModeCheck, "s");                                   \
        FIELD_ACTION(float, elapsedTimeForBillboardSymbolsRendering, "s");                                      \
        FIELD_ACTION(unsigned int, billboardSymbolsRendered, "");                                               \
        FIELD_ACTION(float, elapsedTimeForOnPathSymbolsRendering, "s");                                         \
        FIELD_ACTION(unsigned int, onPathSymbolsRendered, "");                                                  \
        FIELD_ACTION(float, elapsedTimeForOnSurfaceSymbolsRendering, "s");                                      \
        FIELD_ACTION(unsigned int, onSurfaceSymbolsRendered, "");                                               \
        \
        /* Time elapsed for debug stage */                                                                      \
        FIELD_ACTION(float, elapsedTimeForDebugStage, "s");                                                     \
        FIELD_ACTION(float, elapsedTimeForDebugRects2D, "s");                                                   \
        FIELD_ACTION(float, elapsedTimeForDebugLines2D, "s");                                                   \
        FIELD_ACTION(float, elapsedTimeForDebugLines3D, "s");                                                   \
        FIELD_ACTION(float, elapsedTimeForDebugQuad3D, "s");

        struct OSMAND_CORE_API Metric_renderFrame : public IMapRenderer_Metrics::Metric_renderFrame
        {
            Metric_renderFrame();
            virtual ~Metric_renderFrame();
            virtual void reset();

            OsmAnd__AtlasMapRenderer_Metrics__Metric_renderFrame__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const bool shortFormat = false, const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_METRICS_H_)
