#include "IMapRenderer_Metrics.h"

QString OsmAnd::IMapRenderer_Metrics::Metric_update::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += prefix + QString(QLatin1String("elapsedTime = %1s\n")).arg(elapsedTime);
    output += prefix + QString(QLatin1String("elapsedTimeForUpdatesProcessing = %1s\n")).arg(elapsedTimeForUpdatesProcessing);
    output += prefix + QString(QLatin1String("elapsedTimeForRenderThreadDispatcher = %1s")).arg(elapsedTimeForRenderThreadDispatcher);

    return output;
}

QString OsmAnd::IMapRenderer_Metrics::Metric_prepareFrame::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += prefix + QString(QLatin1String("elapsedTime = %1s")).arg(elapsedTime);

    return output;
}

QString OsmAnd::IMapRenderer_Metrics::Metric_renderFrame::toString(const QString& prefix /*= QString::null*/) const
{
    QString output;

    output += prefix + QString(QLatin1String("elapsedTime = %1s")).arg(elapsedTime);

    return output;
}
