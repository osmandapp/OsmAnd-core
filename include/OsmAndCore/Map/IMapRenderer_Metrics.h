#ifndef _OSMAND_CORE_I_MAP_RENDERER_METRICS_H_
#define _OSMAND_CORE_I_MAP_RENDERER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Metrics.h>

namespace OsmAnd
{
    namespace IMapRenderer_Metrics
    {
#define OsmAnd__IMapRenderer_Metrics__Metric_update__FIELDS(FIELD_ACTION)               \
        /* Total elapsed time */                                                        \
        FIELD_ACTION(float, elapsedTime, "s");                                          \
                                                                                        \
        /* Time elapsed for MapRendererResourcesManager::checkForUpdatesAndApply()  */  \
        FIELD_ACTION(float, elapsedTimeForUpdatesProcessing, "s");                      \
                                                                                        \
        /* Time elapsed to process all scheduled calls in render thread */              \
        FIELD_ACTION(float, elapsedTimeForRenderThreadDispatcher, "s");
        struct OSMAND_CORE_API Metric_update
        {
            inline Metric_update()
            {
                reset();
            }

            virtual ~Metric_update()
            {
            }

            inline void reset()
            {
                OsmAnd__IMapRenderer_Metrics__Metric_update__FIELDS(RESET_METRIC_FIELD);
            }

            OsmAnd__IMapRenderer_Metrics__Metric_update__FIELDS(EMIT_METRIC_FIELD);
            
            virtual QString toString(const QString& prefix = QString::null) const;
        };

#define OsmAnd__IMapRenderer_Metrics__Metric_prepareFrame__FIELDS(FIELD_ACTION)         \
        /* Total elapsed time */                                                        \
        FIELD_ACTION(float, elapsedTime, "s");
        struct OSMAND_CORE_API Metric_prepareFrame
        {
            inline Metric_prepareFrame()
            {
                reset();
            }

            virtual ~Metric_prepareFrame()
            {
            }

            inline void reset()
            {
                OsmAnd__IMapRenderer_Metrics__Metric_prepareFrame__FIELDS(RESET_METRIC_FIELD);
            }

            OsmAnd__IMapRenderer_Metrics__Metric_prepareFrame__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const QString& prefix = QString::null) const;
        };

#define OsmAnd__IMapRenderer_Metrics__Metric_renderFrame__FIELDS(FIELD_ACTION)          \
        /* Total elapsed time */                                                        \
        FIELD_ACTION(float, elapsedTime, "s");
        struct OSMAND_CORE_API Metric_renderFrame
        {
            inline Metric_renderFrame()
            {
                reset();
            }

            virtual ~Metric_renderFrame()
            {
            }

            inline void reset()
            {
                OsmAnd__IMapRenderer_Metrics__Metric_renderFrame__FIELDS(RESET_METRIC_FIELD);
            }

            OsmAnd__IMapRenderer_Metrics__Metric_renderFrame__FIELDS(EMIT_METRIC_FIELD);

            virtual QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_METRICS_H_)
