#ifndef _OSMAND_CORE_I_MAP_RENDERER_METRICS_H_
#define _OSMAND_CORE_I_MAP_RENDERER_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>

namespace OsmAnd
{
    namespace IMapRenderer_Metrics
    {
        struct OSMAND_CORE_API Metric_update
        {
            inline Metric_update()
            {
                reset();
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_update));
            }

            // Total elapsed time
            float elapsedTime;

            // Time elapsed for MapRendererResourcesManager::checkForUpdatesAndApply()
            float elapsedTimeForUpdatesProcessing;

            // Time elapsed to process all scheduled calls in render thread
            float elapsedTimeForRenderThreadDispatcher;

            QString toString(const QString& prefix = QString::null) const;
        };

        struct OSMAND_CORE_API Metric_prepareFrame
        {
            inline Metric_prepareFrame()
            {
                reset();
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_prepareFrame));
            }

            // Total elapsed time
            float elapsedTime;

            QString toString(const QString& prefix = QString::null) const;
        };

        struct OSMAND_CORE_API Metric_renderFrame
        {
            inline Metric_renderFrame()
            {
                reset();
            }

            inline void reset()
            {
                memset(this, 0, sizeof(Metric_renderFrame));
            }

            // Total elapsed time
            float elapsedTime;

            QString toString(const QString& prefix = QString::null) const;
        };
    }
}

#endif // !defined(_OSMAND_CORE_I_MAP_RENDERER_METRICS_H_)
